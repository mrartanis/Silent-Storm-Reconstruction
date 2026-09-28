# Native mission creation and post-init (stage 2)

The expanded parallel GCC Linux x86-64 suite on 2026-09-27 exposed another
undefined value on authored root missions 3829 and 4526: a newly created
`CNodesLayer::SLadder` had `bConsistent` uninitialized when vector growth
copied it. `RefreshLadder` still determines the real state later, but the
copy must not read an indeterminate `bool`. `CreateLaddersInternal` now sets
it to `false` before insertion. Focused `NativeWorldMission3829` and
`NativeWorldMission4526` tests pass again on Windows x64, GCC Linux
x86-64 under ASan/UBSan/LSan, and ARM64/QEMU under ASan/UBSan with leak
detection off. The complete post-fix Windows suite passed 148/148. The
complete post-fix GCC Linux x86-64 suite passed 123/123 under
ASan/UBSan/LSan, including the 52-root extended map test (458.92 seconds
for that case). The complete ARM64/QEMU suite passed 122/122 under
ASan/UBSan with `-LE extended` and leak detection disabled; both authored
mission cases passed there as well. The all-52-root ARM64 extended case
remains separate and is not claimed complete.

`NativeWorldInitProbe` has optional `--mission <variant>`,
`--mission-ui-ack <variant>`, `--mission-base-party-ui-ack <variant>`,
`--mission-party-ui-ack <variant>`, and
`--mission-party-shot <variant>`, and
`--mission-party-shot-save <variant>` and
`--mission-party-shot-slot <variant>`, plus
`--mission-party-explosion-save <variant> <save-file>` and
`--mission-party-grenade-save <variant> <save-file>` and
`--mission-party-grenade-flight-save <variant> <save-file>` and
`--mission-party-grenade-inventory-save <variant> <save-file>` and
`--mission-party-eng-grenade-inventory-save <variant> <save-file>` paths. It loads the
original `game.db` and four autoload scripts, constructs the original `CWorld`
with an RPG global game, calls `CWorld::CreateRandom` with the original
`BuildMap`, then calls `CWorld::RunPostInit`. Variant 218 exercises a small
building map with no units or attached script. Variant 810 adds two authored
units and one 1,016-byte attached Lua script; its startup also runs the
world's first-segment warm-up, vision, physics, and path-colouring jobs.

The tests are `NativeWorldMission218`, `NativeWorldMission810`,
`NativeWorldMission810UIAck`, `NativeWorldBase5376UIAck`,
`NativeWorldMission810PartyUIAck`, and
`NativeWorldMission810PartyShot`, and
`NativeWorldMission810PartyShotSave` and
`NativeWorldMission810PartyShotSlot` and
`NativeWorldMission810PartyExplosionSave` and
`NativeWorldMission810PartyGrenadeSave` and
`NativeWorldMission810PartyGrenadeFlightSave` and
`NativeWorldMission810PartyGrenadeInventorySave` and
`NativeWorldMission810PartyEngGrenadeInventorySave` in both
the Windows and portable CMake builds.
Example Linux verification, from a build
configured with `S2_GAME_DB_PATH`, `S2_RESOURCE_PACKAGE_PATH`, and
`S2_SCRIPT_CORPUS_DIR`:

```sh
cmake --build build-x64 --target NativeWorldInitProbe NativeBuildingGridTests NativeHeightNetworkTests -j 16
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
  ctest --test-dir build-x64 --output-on-failure \
  -R 'NativeWorld(Mission|Init|Base)|NativeBuildingGridTests|NativeHeightNetworkTests'
```

The data package and Lua files must be the same originals used by the other
stage-2 resource probes. The Linux test stages the autoload files under its
working directory; resource packages remain in the configured resource
directory. `ASAN_OPTIONS=detect_leaks=0` is the existing QEMU-compatible
sanitizer setting; UBSan's `halt_on_error=1` is intentional for this gate.

The previous packet passed the full CTest matrix on 2026-09-26: Windows x64
129/129, Linux x86-64 106/106, and Linux ARM64/QEMU 106/106. Both Linux
suites used ASan/UBSan, `ASAN_OPTIONS=detect_leaks=0`, and
`UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1`.
With the UI-ack gate added on 2026-09-27, the complete matrix passed:
Windows x64 130/130, Linux x86-64 107/107, and Linux ARM64/QEMU 107/107
under the same sanitizer settings. This builds only the diagnostic probe on
Linux, not a Linux game executable.
With the deployed-party gate and two UB fixes, the complete matrix passed
again: Windows x64 131/131 (and `Game.exe` built), Linux x86-64 108/108,
Linux ARM64/QEMU 108/108 under ASan/UBSan with the same halt-on-error options.
The ARM64 suite took about 196 seconds, with the party mission probe taking
about 94 seconds. This remains a headless diagnostic Linux build.

The clean Windows x64 native-media archive from source commit `1f99bbd` is
`G:\SS\lab\builds\stage2-party-command-20260927-01`, built with 16 jobs.
`Game.exe` SHA-256 is
`8AFD493648153F033F1E97A9E59D56F6BF5122A54D82154D4CC505A22293DE98`.
The archive has no `fmod.dll`. No separate live-game smoke is claimed for
this archive; the Windows `Game.exe` build and 131/131 CTest are the verified
regressions for this packet.

The first live variant-810 run exposed three previously dormant x64/Linux
undefined behaviors, now covered by the mission gate and local regressions:
`SFlipper` moved an uninitialized `bool` during path-network growth;
building-junction hashes shifted negative signed coordinates; and AI-map hull
queries called methods on absent octree children. The fixed-point vision
rasterizer also relied on signed 32-bit overflow for edge stepping; its
unsigned modulo-32 arithmetic now states the original wraparound explicitly.
The coordinate hashes retain their original signed `int` return values, so
the 64-bit hash table sees the same sign extension as before.

The probe now restores the active Lua stack after reading startup globals.
Without that guard, the diagnostic itself left the numeric constant `7` in
the `TriggersManager` thread's call slot; its later `Sleep(10)` then produced
"attempt to call a number value" even on a script-free map. This was a
probe-induced failure, not evidence of a game Lua defect. After the guard,
both map variants advance 220 world segments without a Lua error. For variant
810 the probe first forces NIGHT and verifies that the authored script's
`SetTimeOfDay(DAY)` changes the world back to DAY during post-init. This is a
specific observed mission-script effect, not merely successful parsing.

The separate `--mission-ui-ack 810` probe advances one segment at a time and
drains the original world UI-command queue. For commands whose IDs are still
tracked by the attached script, it posts the original `CCmdInterfaceEvent`
through `CWorld::ExecuteCommand`, as the real UI would do when it finishes a
command. In this diagnostic mode, three IDs were acknowledged on Windows x64.
The test also requires the authored `OnClickUsable` function to exist after
220 segments. That function is defined *after* the opening camera/sequence
waits, so this proves the Lua thread advanced beyond those waits, rather than
only that the queue was drained. The probe uses `Script::AutoBlock` for this
global lookup so it does not damage the active Lua stack. It does not execute
the visual side of a UI command or validate the result of `UnitShootPrepare`.

`--mission-base-party-ui-ack 5376` broadens the live Lua/data coverage from
the introductory combat map to an authored base. It deploys the hero, advances
one attached script through three UI-command acknowledgements, checks the
`CTemplVariant::bNoAttack` value from the original `game.db` against the
world's combat gate, then asks the hero to shoot a tile. The normal
`CUnitServer::CanDo` path must reject that command with
`UCR_GENERAL_FAILURE` before trajectory evaluation. This covers a distinct
game rule and Lua start sequence; the headless acknowledgements do not
render the base UI or prove a complete base visit. The test exposed an
unowned route-logic allocation while setting up the base:
`CAIUnit::SetRouteLogic` rejected a candidate route, leaving the raw
new object behind. Holding the candidate through the gate and slot swap
releases rejected routes without changing accepted routes. Linux x86-64
passed this test with LeakSanitizer active. Full matrices after the fix:
Windows x64 140/140 (including a rebuilt `Game.exe`), Linux x86-64 117/117,
and Linux ARM64/QEMU 117/117 under ASan/UBSan. The ARM64 matrix took about
460 seconds; the base test took about 143 seconds. Ten extra Windows runs
each of the base gate and the post-load shot gate passed.
The clean native-media x64 archive from source commit `70659eb` is
`G:\SS\lab\builds\stage2-base-noattack-20260927-01`; its `Game.exe`
SHA-256 is `5D9371031806CFE719325C8B1F8176E7EC19118412E220B49B45078AF92B2596`.
It contains no `fmod.dll`. This exact archive has not been separately run
through a graphical game smoke test.

The older 810 probe had no `CGlobalPlayer` or hero. Thus its authored
`GetHero()` returned nil and `UnitShootPrepare(pers1, nil, ...)` was a no-op;
reaching `OnClickUsable` proved Lua scheduling, **not** unit-command dispatch.
The new party mode creates the game's default global player, then follows
`CPlayerTracker`'s mission order: after `CreateRandom` and before `RunPostInit`,
it registers the player with a `CSequenceCommander` and deploys the hero.
The test requires a live hero server and samples `pers1`'s executor after
each segment. With the party, a `CExecQueue` appears; it does not appear in
the otherwise identical no-party probe. The script's `UnitShootPrepare`
therefore reaches the unit execution path in this selected scenario. The
queue's presence does not prove a completed aim, fired projectile, damage,
or Steam-equivalent outcome.

Following that path exposed a gameplay wiring defect: the Lua binding sets
`CCmdShootObject::bOnlyPrepareToShoot`, and `CExecShoot::Start` already has an
aim-and-hold branch for the matching executor flag, but
`CreateActionExecutor` discarded the command flag when constructing
`CExecShootUnit`. The AT_SHOOT/AT_SNIPE and AT_CANNON paths now pass the flag
through the constructor. Ordinary commands still default to `false`.
This corrects the command-to-executor state transfer; the headless gate still
does not observe a completed aim animation or prove visual/Steam parity.
The next headless observation found that map 810's scripted prepare action
completes with HP 137 to 137, AP 46 to 46, ammo 32 to 31, and one
`CEventOnAttackAtUnit`. An initial change suppressed the ammo consumption;
Windows x64 passed 131/131 CTest and Linux x86-64/ARM64 each passed 108/108
under ASan/UBSan. Those tests did **not** prove Steam parity. The resulting
archive is retained only as a record of the rejected experiment:
`G:\SS\lab\builds\stage2-aim-no-ammo-20260927-01` from commit `e892804`;
`Game.exe` SHA-256 is
`B800C1CF07A70A18ADBE5D2AB064EE29D6EA926CB61383FB795BB4CA8CD556BA`.
There is no `fmod.dll`, but this build diverges from the Steam executable's
aim-only path and must not be used as the parity reference.

Static inspection of the unchanged Steam `game.exe` (SHA-256
`4f417593a9f73e2bfde12d83cdbd694ae47eecb92812140d67b8b343e4a95705`)
resolved that question. At VA `0x7a8d60`, `CExecShoot::Start` tests the
aim-only byte at `+0x115` and still calls virtual `SelectRay`. The unit
implementation at `0x7a5480` pushes literal `1` as `bSpendAmmo` into
`CreateAttack` (`0x7a4a00`), even on that aim-only path. At `0x7a9610`,
`OnLabel` skips arming a bullet for aim-only, then the cancellation branch
calls virtual `CheckShotResult`; the unit implementation at `0x7a9880`
sends `OnShotAtUnit` and `CEventOnAttackAtUnit`. Thus the round consumption
and notification are original release behavior, not port bugs. The
`bSpendAmmo=false` experiment was reverted; the regression now checks the
selected original quirk. This static path proof does not replace a live
Steam mission run or visual aim-animation comparison.
With the retail path restored, Windows x64 rebuilt `Game.exe` and passed
131/131 CTest; Linux x86-64 and ARM64/QEMU each passed 108/108 under
ASan/UBSan with `halt_on_error=1`. The clean native-media x64 archive from
corrective commit `06ed84a` is
`G:\SS\lab\builds\stage2-steam-aim-20260927-01`, with `Game.exe` SHA-256
`7CB5D1766707E39EF511BDA1E203F69DBFA14C585DF96D188EAC823E630699E3`.
It contains no `fmod.dll`; no live-game smoke of this archive is claimed.

The separate `--mission-party-shot 810` diagnostic continues the same
headless mission after the scripted aim, then sends the game's normal
`CCmdShootObject` and `CCmdContinue` to `pers1` against the deployed hero.
It advances another 220 segments and observes the unit executor, ammunition,
AP, target HP, `CEventOnAttackAtUnit`, and `CEventOnBullet`. The probe sets
the game's `nScriptToHit` override (the field used by `UnitSetToHit`) to 100 for this controlled
shot, so hit versus miss does not depend on the process-level RNG seed;
damage still varies and is deliberately not byte-for-byte asserted. In five
Windows x64 runs, each shot finished with ammo 31 to 30, AP 46 to 36, one
bullet event, one additional attack event, and positive damage. Linux
x86-64 and ARM64/QEMU reproduced those invariants under ASan/UBSan. This is an actual
headless attack and bullet path, not a rendered battle or Steam comparison.
With this gate registered in both builds, Windows x64 rebuilt `Game.exe` and
passed 132/132 CTest, while Linux x86-64 and ARM64/QEMU passed 109/109 each
under ASan/UBSan with `halt_on_error=1`.
The clean native-media x64 archive from source commit `2764aef` is
`G:\SS\lab\builds\stage2-party-shot-20260927-01`; `Game.exe` SHA-256 is
`22023B3A73F1A19D991788B59B49D087B294463801DE365A2A9943957B5AA46B`.
It contains no `fmod.dll`. A live-game smoke of this archive is not claimed.
The clean native-media x64 archive from source commit `c19b144` is
`G:\SS\lab\builds\stage2-prepare-shot-20260927-01`; its `Game.exe` SHA-256
is `515177BA2C38C3C34952E2E189492E72E58CC523C80BFCDE0F392CB3B1E40207`.
It contains no `fmod.dll`. No separate live-game smoke or direct Steam
comparison is claimed for this archive.

`--mission-party-shot-save 810` continues the fired-shot scenario and uses
the game's `CFileStream` and `CStructureSaver` to write the live `CWorld`
object with tag 2 followed by `SerializeShared`, in the same payload order
as `CMission::SaveWorld`. It reads the file into a second `CWorld` under
`CSharedHolder` and restores the shared caches. The test's file lives only
under its build directory, not the baseline game or a user save slot.
The regression
compares world time and time of day, the deployed hero's HP, the shooter's
ammunition and AP, and the presence of the mission's `OnClickUsable` Lua
function after restoration. The probe replaces its original `CWorld` and
`CGlobalGame` references with the loaded instances, advances ten further
world segments, and requires time to progress with both units still present
and no Lua error. It then gives the restored shooter a second normal
`CCmdShootObject`/`CCmdContinue` against the restored hero. The regression
requires a new executor, one spent round, ten spent AP, no spontaneous HP
increase, and one new attack and bullet event. `ScriptToHit=100` fixes the
hit roll but cannot force a ray to penetrate cover: a valid second shot
occasionally leaves HP unchanged. The first pre-save shot still requires
positive damage. The selected Windows run observed
ammo 30 to 29, AP 36 to 26, and hero HP 113 to 85; damage is not fixed
because it uses the game's RNG. Windows x64, Linux x86-64 and ARM64/QEMU pass
this round-trip; both Linux targets use ASan/UBSan with `halt_on_error=1`.
This invokes the production file serializer and shared-cache payload, but
not `CMission::SaveWorld`/`LoadWorld` themselves: the save manager's active
slot and game/UI shell are not involved in this direct-file mode. The second
controlled shot proves that combat can execute on the newly loaded world,
not a route decision,
full-mission continuation, or Steam-equivalent combat outcome. This does not
prove Steam save compatibility.
On Linux this deeper read initially found a null `CBuildInfoLoader` in
shared cache ID 108: its save/load registration was still owned by the
renderer-dependent `GBuilding.cpp`. The existing headless `BuildingInfo.cpp`
now registers the same retail class ID only on non-Windows builds. The
production Windows registration is unchanged.
The clean Windows x64 native-media archive from source commit `be79d00` is
`G:\SS\lab\builds\stage2-party-shot-file-20260927-01`; its `Game.exe`
SHA-256 is
`282EA904AAC5E75EAC7443F462418827C3A7DE5A9D2F37F84B60144876672273`.
The archive contains no `fmod.dll`. No live-game smoke of this specific
archive is claimed.

This deeper serialization exposed previously hidden uninitialized state in
`CUnitAnimator::bIdle`, door chest/transparency flags and empty trap data,
`CColouredWaysCalcer::bPrevStandOnly`, `SSkeletonState`'s scalar fields,
`SModifiable<bool>::data`, and the default `CVolumeNode::STrackerDescr`
copied by the list loader. Those fields now have deterministic initial values.
On Linux, the first restored world lacked its `CCTime` because the original
`Main/Time.cpp` save/load registration was not linked into the headless
probe. The probe now compiles that original translation unit. These fixes
are required for the actual world-save graph, not a new save format.
After the new gate and fixes, the full matrix passed: Windows x64 133/133
with `Game.exe` built, Linux x86-64 110/110, and ARM64/QEMU 110/110. Both
Linux suites used ASan/UBSan with `halt_on_error=1`; the ARM64 suite took
about 279 seconds with eight concurrent tests. These counts do not imply
a complete Linux game executable or a live Steam parity run.
The extended post-load shot gate passed the complete Windows x64 133/133,
Linux x86-64 110/110, and ARM64/QEMU 110/110 suites. Both Linux suites used
ASan/UBSan with `halt_on_error=1`; the ARM64/QEMU suite took about 287 seconds.
Five additional Windows repetitions of the save/restore/second-shot test passed.

`--mission-party-shot-slot 810` follows the same mission and combat path,
but writes the world payload through `CSaveManager::GetSlotFilePathW` in its
active `temp` slot. It then calls `SaveSlot` to copy that slot, clears the
active slot, calls `LoadSlot` on the named copy, and reads the restored active
file before continuing the world and firing the second shot. The test
requires `S2_USER_DATA_DIR`; CTest sets it to `world-slot-user` inside the
build tree, so no baseline or real user save directory is touched. The
Windows x64 targeted gate and complete 134/134 suite passed; Linux x86-64
and ARM64/QEMU passed their targeted gates and complete 111/111 suites under
ASan/UBSan with `halt_on_error=1`. The ARM64 suite took about 294 seconds.
Five additional Windows repetitions of the slot path passed.
This integrates the original slot manager and serializer but
still does not instantiate the renderer-dependent `CMission`, invoke its
`SaveWorld`/`LoadWorld` methods, or run the game's save UI.

The slot gate now also hands control to the enemy commander after the loaded
world's second controlled shot, removes the diagnostic forced-hit override,
and advances the original AI/turn loop until it observes a new bullet event
and ammunition consumption from the enemy's autonomous shot. The same test
checks that `CAIUnit::pAIState` points at its owning commander's embedded
`SAIState` before and after save/load. This pointer is transient and was
previously null after load: `CAICommander::ReconnectWorld` now reattaches it
only for that commander's own units while restoring world/state back-references.
The diagnostic calls `CWorld::RestoreRuntimeCaches`, the production callback
used by `CMission::OnSnapshotRestored`, after deserialization. This exercises
an actual AI choice and attack in the restored world; it does not establish
which target or route Steam would select in the same random state.

The longer AI turn exposed another omitted headless save/load registration:
building placement objects of class `CFBTransform` (retail ID `0x025a1130`)
were read as null because `Main/DiscretePos.cpp` was absent from the Linux
probe. The probe now compiles the original translation unit, preserving its
Windows registration. It also exposed an indeterminate `SUnitPosition::bRun`
in an unavailable Panzerklein-terror action's cached `SInfo`; the position
now defaults that flag to false before the cache copies it. These are
game-used world/AI paths, not editor features or graphics substitutes.
After these changes, Windows x64 built `Game.exe` and passed the full 134/134
CTest suite; Linux x86-64 and ARM64/QEMU each passed 111/111 with ASan/UBSan
and `halt_on_error=1`. The Windows slot test also passed five consecutive
repetitions. The ARM64 full suite took 286 seconds.
The clean Windows x64 native-media archive from source commit `eab7fa7` is
`G:\SS\lab\builds\stage2-ai-resume-20260927-01`; its `Game.exe` SHA-256 is
`3EC1BC10B54C0E117D4B826DC5B47C348CA37DDA3C24847B57DDB439A5D103A8`.
It contains no `fmod.dll`. A separate live-game smoke of this particular
archive is not claimed.
The preceding memory-stream gate's clean Windows x64 native-media archive
from source commit `0dae7ca` is
`G:\SS\lab\builds\stage2-party-shot-save-20260927-01`; its `Game.exe`
SHA-256 is
`F7CEDA604823F4D43D97DA623367D3EC7EF7D605A2730890B2FDE854600A6C05`.
The archive includes the native-media DLLs and no `fmod.dll`. Its build
metadata names the source commit and records a clean worktree. A separate
live-game smoke of this archive is not claimed.

This newly exercised path exposed two previously hidden UB cases on Linux:
`SUnitDeployData` left `bCorpseAlive` and `bCorpseEnemy` indeterminate when
adding a mercenary, and `NAI::CPath` left `bStrafePath` indeterminate before
`CExecQueue::AddPath` read it. Both now initialize to false. The original
game's tiny `RPGMerc.cpp` implementation of `CreateMerc` is included in the
portable RPG target so this is an actual game party, not a fabricated hero.

This is a headless original-world startup, **not** a full Linux game. The
party mode deploys a hero and exercises one scripted aim-only command, a
controlled fired shot, and a file-backed `CWorld`/shared-cache round-trip
followed by ten headless segments and a second controlled shot on the loaded world.
The alternate slot mode also copies and reloads the world through the active
`CSaveManager` slot and exercises one autonomous enemy attack. A separate
`--mission-party-explosion-save 810` mode invokes the game's `CWorld::Explode`
at the centre of a generated building, then serializes the world and reads it
back. The test uses a read-only diagnostic view of the building's voxel grid:
live destructible cells, their HP sum, and a deterministic hash of *every*
cell and the grid dimensions. In the selected mission-810 building on Windows
x64, the explosion changed 108 live cells to 79, HP sum 18838 to 10173, and
hash `93041fa732f2d5e4` to `8a7e7a9b02af9c19`; the loaded grid had the
same 79 cells, 10173 HP sum and `8a7e7a9b02af9c19` hash. Linux x86-64 and
ARM64/QEMU produced the same numbers and hashes under ASan/UBSan. The explosion also
creates a temporary skeleton animator with no skeleton; ASan/UBSan exposed
uninitialized serialized `bServer`/`bItem` flags on that path, now initialized
to false. This calls the world explosion directly, not an inventory grenade,
rocket trajectory or authored script trigger. It verifies voxel damage and
its persistence, not equivalence of the blast, stability cascade, FX, or AI
route changes to Steam.
With the voxel-hash gate, Windows x64 built `Game.exe` and passed 135/135
CTest; Linux x86-64 and ARM64/QEMU each passed 112/112 under ASan/UBSan with
`halt_on_error=1`. Five consecutive Windows runs of the explosion test passed.
The final ARM64 suite took 297 seconds. This remains a headless world probe,
not a Linux `Game.exe` or GPU check.
The clean Windows x64 native-media archive from source commit `ad3412b` is
`G:\SS\lab\builds\stage2-voxel-explosion-20260927-01`; its `Game.exe`
SHA-256 is
`A4D9623716A2C8C291D17CC3E4189ECFA68C9C33ACC191DD4332DBBF623C6D72`.
It contains no `fmod.dll`. A live-game smoke of this specific archive is not
claimed; the headless explosion test uses the same game code but does not
exercise the UI or GPU.

`--mission-party-grenade-save 810` advances beyond the direct explosion:
it enqueues original grenade record 21 through `CWorld::AddGrenadeExplosion`
with the mission shooter as thrower, then lets the original
`CExplosionMaster::Segment`/`CVoxelExpl::ApplyWaveDamage` reach the building's
RPG attackable and grid. Record 21 has one wave, 60–100 wave damage, radius
1.00 and structure coefficient 1.10 in the selected original `game.db`.
The test requires a lower live-cell count and HP sum, then an exact
post-save/post-load cell hash; it never edits the voxel grid itself.
It uses `SeedForHarness(81021)` just before enqueuing to fix process entropy.
That does **not** force identical damage: the blast-object registry is hashed
by pointer-derived keys, and iteration assigns random damage rolls to
objects in an address-dependent order. Ten Windows x64 repetitions under
this seed left 106–107 live cells (from 108), with an exact restored hash in
each run; five Linux x86-64 repetitions left 106, and one ARM64/QEMU run left
106, also with exact reload hashes. This is a genuine normal-grenade damage
and persistence gate, not proof of the same per-cell outcome on every run
or of parity with Steam's observable explosion. A thrown item's inventory
command, flight/fuse, engineer grenades and post-blast AI routing are not
covered by this diagnostic.
After adding this gate, the full matrix passed on 2026-09-27: Windows x64
136/136 (with `Game.exe` built), Linux x86-64 113/113 and Linux ARM64/QEMU
113/113 under ASan/UBSan. The ARM64 full run took about 302 seconds.
The stricter live-cell-loss assertion subsequently passed on Windows x64,
Linux x86-64 and ARM64/QEMU (targeted test; ARM64 took 98 seconds).
The clean native-media x64 archive from `46bf268` is
`G:\SS\lab\builds\stage2-grenade-wave-20260927-01`; its `Game.exe`
SHA-256 is `4DC33EE20E860259BA53D08183F2F450297C3307892C7A67C320803D5FCA484D`.
It contains no `fmod.dll`. A live-game smoke of this specific archive is
not claimed.

`--mission-party-grenade-flight-save 810` advances one step further: it calls
the original `UnitThrowGrenade` path used by the game's Lua command handlers.
That path creates a transient grenade item, calculates a skill-limited
ballistic arc and scatter, then spawns `CGrenadeServer` through
`CWorld::ThrowGrenade`. In the selected map, the contact-fused record 21
flies from `pers1` toward the building. The test advances 200 segments,
requires actual live-cell and HP loss, then checks the exact voxel-grid
round-trip after world save/load. It does not equip an inventory grenade,
spend action points, or run the player's throw animation.

The first headless attempt spawned the flying object but left it active for
all 200 segments, with no damage. `CASphereSet::DidCollide()` observes a
collision only after the lazy physics animator has been refreshed; the
renderer normally causes such a refresh, but headless world segments did not.
`CGrenadeServer::Segment` now refreshes its animator before checking a
contact fuse. Timed fuses retain their existing path. This makes collision
and detonation part of the simulation tick rather than a side effect of
rendering. Dynamic parity of the final scatter/damage with Steam is still
unverified. The new test passed 20/20 Windows x64 repetitions; with it,
the full Windows matrix passed 137/137 (and `Game.exe` built), Linux
x86-64 passed 114/114 and Linux ARM64/QEMU passed 114/114 under ASan/UBSan.
The ARM64 suite took about 378 seconds, with the flight gate taking about
100 seconds.
The clean native-media x64 archive from `ec4f587` is
`G:\SS\lab\builds\stage2-grenade-flight-20260927-01`; its `Game.exe`
SHA-256 is `7C70C50B0F8B6A0F33B0744A2E86B5BAD56DB721089ABDAF5E78EE81F770332B`.
It contains no `fmod.dll`. This archive was not separately smoke-tested in
the graphical game.

`--mission-party-grenade-inventory-save 810` equips a real grenade item from
record 21 in the mission shooter's second inventory slot, activates it, and
issues the normal `CCmdShootTile` followed by `CCmdContinue`. The continue
command is essential: the first command creates `CExecQueue` but does not
start its action, exactly as in the existing player-shot gate. The test
requires the grenade item to leave the slot, 20 AP to be spent, voxel-cell
and HP loss in the target building, and exact grid restoration after save/load.
This covers inventory, action executor, ballistic flight, contact fuse,
explosion and voxel persistence without using the UI. The first diagnostic
attempt omitted `CCmdContinue`, leaving AP and inventory unchanged; that was
a harness error, not a game bug. With the complete command sequence, 20/20
Windows x64 repetitions passed. The full matrices passed Windows x64 138/138
(with `Game.exe` built), Linux x86-64 115/115 and Linux ARM64/QEMU 115/115
under ASan/UBSan. The ARM64 suite took about 395 seconds, with this gate
taking about 99 seconds.
The visual throw animation, Steam's dynamic damage distribution and other
grenade types remain outside this gate.
The clean native-media x64 archive from `d1e5403` is
`G:\SS\lab\builds\stage2-grenade-inventory-20260927-01`; its `Game.exe`
SHA-256 is `0C3A128901F03CAC4AA23EA8C440E86684C400B62D58E7E39FA04D3D04E72E43`.
It contains no `fmod.dll`. A separate graphical smoke of this archive is
not claimed.

`--mission-party-eng-grenade-inventory-save 810` exercises the distinct
engineer-grenade record 2 (item 433). The authored shooter has only 15
engineering, so the isolated test raises that skill to 100 and checks that
`CanDo` accepts the shortened target; the farther building center is out of
ballistic range for this item. It equips and throws via the same inventory
command route, then verifies one item and 20 AP spent, voxel-cell/HP loss in
the building, and exact damaged-grid restoration after save/load. The first
rotation produces the game's `TBS_CANCEL_ACTION`, so the test reissues the
still-unspent action once, just as a player can after an interrupt. The
precise visibility/trap notice causing the cancellation is not isolated; the
test does not establish an engineer-grenade-specific cancellation
bug. Windows x64 passed 20/20 repetitions and the full 139/139 CTest matrix;
Linux x86-64 and ARM64/QEMU passed 116/116 each; the ARM64 run under
ASan/UBSan took about 410 seconds. The graphical throw and Steam's dynamic
outcome remain unverified.

The probe does not create the mission UI, invoke `CMission::SaveWorld`/
`LoadWorld`, or compare dynamic AI, route, battle, and destruction decisions
against Steam x86. The variant-810 script
waits on sequence/UI actions: only the diagnostic ID-acknowledged path reaches
its later callback declaration. No real UI action or later callback invocation
is proved. The absence of Lua errors and these script
effects do not prove that every mission binding worked. Those are the next
stage-2 checks. SDL3/bgfx integration remains in stages
3-4, and no macOS result is claimed.

On 2026-09-27 the `--mission` coverage was broadened using actual variant
metadata from the original `game.db`; that selection alone did not prove
game reachability. An exploratory
Windows x64 run of 20 variants (2223, 7807, 4526, 901, 7962, 3829, 810,
895, 3791, 956, 5006, 1393, 1684, 2216, 2258, 2400, 3844, 3833,
3145, 3845) passed `CreateRandom`, `RunPostInit` and the initial world
advance without Lua errors. Linux x86-64/ASan additionally passed variants
2223, 3829, 4526 and 5006. Variants 2223 (32x32, scripted units and
waypoints) and 4526 (80x96, 49 units, 24 waypoints, scripted patrol map)
are now permanent `NativeWorldMission*` CTest cases on Windows and Linux.
The Windows x64 full suite passed 143/143, GCC Linux x86-64/ASan+UBSan+LSan
passed 120/120, and the two new cases passed on Clang Linux x86-64 with
ASan+UBSan+LSan and on GCC ARM64/QEMU with ASan+UBSan (leak detection off).
The ARM64/QEMU variant-4526 case took 262 seconds, so its timeout is 600
seconds; this is a diagnostic-runtime allowance, not a gameplay timeout.
These tests broaden data and Lua startup coverage, but neither execute all
4726 template variants nor establish complete campaign/AI parity with Steam.
The later `NativeMapDatabaseTests --roots` graph shows 4526 is a direct
scenario root while 2223 is not in that graph. A second direct root, 3829
(112x112, 35 units, 9 waypoints and a script), is now a permanent
`NativeWorldMission3829` CTest on Windows x64, GCC Linux x86-64 and
ARM64/QEMU, and Clang Linux x86-64. The ARM64/QEMU test passed under
ASan/UBSan in about 270 seconds with a 600-second diagnostic timeout.
This proves `CreateRandom`, `RunPostInit` and initial world advance for
that authored root, not completion of its mission or Steam-equivalent AI.

The 2026-09-27 scenario-root world sweep discovers the 52 active authored
roots from `NativeMapDatabaseTests --roots` and runs the production
`CWorld::CreateRandom`/`RunPostInit`/initial advance for each in a fresh
`NativeWorldInitProbe` process. `NativeScenarioRootWorlds` is an `extended`
CTest on Windows x64, Linux x86-64 and ARM64/QEMU. After the fix, Windows
x64 completed 52/52 in about 104 seconds; its 148 non-extended tests also
passed. Linux x86-64 under ASan/UBSan/LSan completed the first 27 roots
through 5175, then stopped at 5240 on an out-of-bounds AI sound-radius read.
After the fix, the remaining 25 roots from 5240 through 7254 passed under
the same sanitizers; its 123 non-extended tests passed after the fix. This
is full root startup coverage across the two Linux runs, not a single
uninterrupted post-fix CTest run. At this first fallback revision the full
ARM64 sweep had not been run; its focused 5240 CTest passed under
ASan/UBSan with leak detection disabled in about 156 seconds.

The failure was a step sound emitted during unit movement. Armor-provided
`nAISoundType=5` reached `CAISound::GetRadiusFromAISoundType`, while the
record has only `R1`–`R5` (`vRadius[0..4]`). The accessor now preserves the
old mapping for 0–4 and returns zero for an out-of-range type, without
reading adjacent object memory. The focused
`NativeWorldMission5240` CTest permanently exercises this path on all three
targets. As with the other startup tests, this sweep does not complete those
missions or compare their dynamic AI decisions with Steam.

Static inspection of the unmodified Steam x86 `Game.exe` (SHA-256
`4f417593a9f73e2bfde12d83cdbd694ae47eecb92812140d67b8b343e4a95705`)
found the corresponding accessor at VA `0x80F220`. Its variable-radius
branch at `0x80F252` executes `fld DWORD PTR [ecx+eax*4+0x14]` without a
bounds check. At index 5 that addresses the adjacent `pSound` pointer,
not an authored `R6`. The Gold PDB-located function at VA `0x820520`
has the same addressing instruction. This establishes the retail binary's
unchecked lookup. A CDB breakpoint in the restored x64 probe, while
advancing root 5240, caught `nAISoundType=5` on original-db AISound record
ID 5 and read a null `pSound` field. The database regression now asserts
this field is null and the safe type-5 return is zero on Windows x64,
Linux x86-64 and ARM64. Applying the Steam x86 instruction to that same
original record therefore yields zero; this is an inference from the
retail instruction and record data, not a live Steam AI-response measurement.

The clean native-media x64 archive from commit `7ef0211` is
`G:\SS\lab\builds\stage2-scenario-worlds-20260927-01`; `Game.exe`
SHA-256 is
`26143B94C50F31C7893E65CCC8C9C9840EE5C6A7FB71F37140C5818F9E715016`.
It was built with 16 jobs and has no `fmod.dll`. The isolated
`stage2-scenario-worlds-smoke-20260927-01` run reached a responsive,
rendered main menu (capture `evidence/menu-focused.png`); the game closed
normally, no crash dump appeared, and its `Game.exe` hash matches the
archive. This is a boot/menu smoke, not a mission gameplay check. This
archive predates the subsequently corrected zero-radius fallback; retain
it as evidence for commit `7ef0211`, not as the latest gameplay build.

For the zero-radius revision, Windows x64 passed all 148 non-extended
tests and the 52-root `NativeScenarioRootWorlds` again (about 106 seconds).
GCC Linux x86-64 with ASan/UBSan/LSan passed all 123 non-extended tests
and a single uninterrupted 52-root CTest (about 987 seconds). The
revised ARM64/QEMU sweep also passed all 52 roots with ASan/UBSan and leak
detection disabled. It used the nine disjoint numeric ranges below in
parallel, each invoking the same script and a fresh QEMU process per root;
this is complete root coverage on the final zero-radius binary, but not
a single serial ARM64 CTest invocation. The earlier all-root sweep built
with the `R5` fallback is not being used as evidence for the final code.
The permanent `NativeWorldMission5240` ARM64 CTest was also rerun on the
zero-radius binary and passed in about 155 seconds.
The full ARM64/QEMU non-extended CTest suite then passed 123/123 under
ASan/UBSan with leak detection disabled (300.82 seconds); this includes
the permanent 5240 test and the database accessor regression.

The clean native-media x64 archive of the zero-radius source commit
`f3435db` is
`G:\SS\lab\builds\stage2-scenario-worlds-retail-sound-20260927-01`;
`Game.exe` SHA-256 is
`A101009861A8FE3F49288008A83F3E1E9CD46890C1595BFC7EA61FDF4146C4BD`.
It was built with 16 jobs and contains no `fmod.dll`. The isolated
`stage2-retail-sound-smoke-20260927-01` run reached the rendered,
responsive main menu (`evidence/menu.png`), then closed normally without
a crash dump. The run's `Game.exe` hash matched the archive. This verifies
boot and menu only, not gameplay in root 5240.

Reproduce the complete root startup sweep with
`ctest --test-dir <build> -C RelWithDebInfo --output-on-failure -R '^NativeScenarioRootWorlds$'`
on Windows, or omit `-C` on Linux. On cross-compiled ARM64, that CTest
forwards `CMAKE_CROSSCOMPILING_EMULATOR` into the same script. For parallel
diagnosis under QEMU, run `RunScenarioRootWorlds.cmake` in the configured
`world-probe-root` with `DB_TEST`, `WORLD_PROBE`, `GAME_DB`, `RESOURCE_DIR`,
`TEST_EMULATOR=/usr/bin/qemu-aarch64-static`, and optional
`START_VARIANT`/`END_VARIANT`. Set
`QEMU_LD_PREFIX=/usr/aarch64-linux-gnu`,
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1`. The disjoint numeric ranges used here are
3791–3842, 3844–4526, 5003–5084, 5112–5175, 5240–5271, 5376–5393,
5401–5484, 5709–6277 and 6814–7254; they contain all 52 active roots
from the original `game.db`.

## Root worlds with a deployed party

`NativeScenarioRootPartyWorlds` repeats the 52 active root variants with a
real `CGlobalPlayer`, `CPlayer`, hero unit and sequence commander. It runs
220 world updates at 50-ms intervals and acknowledges only UI-action IDs
that the headless Lua script is waiting for. It checks that world time
advances and Lua reports no error; it does not simulate rendering, camera
motion, input, a complete turn, or Steam's tactical decisions. Each root
still runs in a fresh process. The focused non-extended
`NativeWorldMission3832PartyUIAck` and
`NativeWorldMission6814PartyUIAck` protect the first two sanitizer failures.
Windows x64 passed the 52-root party CTest in 99.76 seconds and all
149 non-extended tests after the first fix. After both fixes it passed
150/150 non-extended tests and the 52-root party CTest again (100.30
seconds). On the final two-fix binary, Linux x86-64 also passed all 52
party roots under ASan/UBSan/LSan in nine disjoint ranges of the same
script. The rebuilt Linux x86-64 non-extended suite passed 123/123; the
new focused 3832 and 6814 modes were run directly because this scratch
build keeps `game.db` and resources at its root instead of the `res/`
layout required for their CTest registration. This is complete root
coverage, not a single serial Linux CTest run.

Linux x86-64 ASan/UBSan found the failure at root 3832:
`CWeaponItem::IsShootModeSupported` indexed `shootModes[100]`, although
`CRPGWeapon` has only six shoot-mode flags. The authored Lua script for
this scenario passes both `100` and `-1` to `UnitSetShootMode` for its
Rocketeer and Laser units. The accessor now rejects out-of-range modes
before indexing, leaving all six valid modes unchanged. This makes the
scripted requests no-ops instead of reads into adjacent object memory;
it is a safety interpretation, not a measured Steam gameplay comparison.
The focused 3832 run passed afterward on Linux x86-64 with ASan/UBSan/LSan
and ARM64/QEMU with ASan/UBSan (LSan disabled).

The first ARM64 sweep of the new party mode also exposed a separate
UBSan error at root 6814, reproduced on Linux x86-64. Authored Lua calls
`CreateAndActivateItem` when no old hand item needs moving. Its command
still copies the default `SItem` source/target records, whose placement
enum, slot and position were uninitialized. `SItem` now gives these
inactive fields neutral defaults (`VACUUM`, `-1`, `(-1,-1)`); explicit
placement constructors continue to override the relevant fields. The
Windows x64 focused 6814 run passed after this edit. This is an
initialization fix, not evidence of Steam inventory parity. The focused
6814 run also passed on Linux x86-64 under ASan/UBSan/LSan and ARM64/QEMU
under ASan/UBSan (LSan disabled) after the edit.
The rebuilt ARM64/QEMU non-extended suite passed 123/123 under
ASan/UBSan with leak detection disabled (376.56 seconds). As on Linux
x86-64, the two new focused tests were run directly in this scratch
resource layout rather than registered CTest names.
The final ARM64/QEMU binary also passed all 52 party-root worlds under
ASan/UBSan (LSan disabled) in the same nine disjoint ranges, with a
fresh QEMU process per root. This is full startup-and-220-tick coverage
of the active roots on ARM64, not a single serial CTest run and not a
Steam gameplay-parity check.

Run `ctest --test-dir <build> -C RelWithDebInfo --output-on-failure
-R '^NativeScenarioRootPartyWorlds$'` on Windows, or omit `-C` on a Linux
build configured against a game directory with `res/Waypoints.res`. The
same `RunScenarioRootWorlds.cmake` used above accepts `-DPARTY_MODE=ON`
and the nine disjoint variant ranges for direct headless Linux/ARM64
diagnosis when the resource files live at the scratch root.

The clean Windows x64 native-media archive from source commit `ed2215e`
is `G:\SS\lab\builds\stage2-party-root-worlds-20260927-01`.
`Game.exe` SHA-256 is
`2D798C053FE1EC8C650A5AB8BB9910951946700B2EF030EC97469002EF337473`;
the archive contains no `fmod.dll`. Its isolated run
`stage2-party-root-worlds-smoke-20260927-01` reached the rendered,
responsive main menu (`evidence/menu.png`); the input helper accepted
cursor movement, and the window then closed normally with no crash dump.
The run's executable hash matched the archive. This is a boot/menu
smoke test, not an in-game tactical or audio-parity result.

## Party-root save and resume sweep

`NativeScenarioRootPartySaveWorlds` extends the 52-root party test: after
220 world updates it writes the real `CWorld` and `SerializeShared` graph,
loads it into another `CWorld`, compares time, time of day and hero HP,
restores runtime caches, then advances ten more world updates while
acknowledging pending scripted UI IDs. Each root uses a fresh process and
its own temporary save file under the build directory. The probe does not
invoke the mission UI or `CMission::SaveWorld`, and these checks do not
establish Steam gameplay parity (stage 7).

The first Windows x64 sweep failed at root 4522 with `0xc0000409` during
Lua-thread serialization. CDB showed `lua_AddString` attempting to construct
a `std::string` from a stale string pointer in a `TObject` stack slot.
`CLuaThread` serialized its entire reserved stack vector, including slots
past `top` that are not live VM state and may retain GC-freed strings.
`TObject` now initializes and clears nil values, and the thread normalizes
only its unused tail to nil before writing. The active stack and its
serialization are unchanged. Root 4522 then passed save/load/resume on
Windows x64 and Linux x86-64 under ASan/UBSan/LSan, and ARM64/QEMU under
ASan/UBSan with leak detection disabled. The full Windows x64 sweep passed
52/52, with the non-extended CTest suite at 150/150 after a complete rebuild
of `Game.exe` and all tests.

Run the full Windows sweep with `ctest --test-dir <build> -C RelWithDebInfo
--output-on-failure -R '^NativeScenarioRootPartySaveWorlds$'`. On Linux,
use the same test without `-C` when `S2_GAME_DIR` contains `res/Waypoints.res`.
In a scratch layout with `game.db` and `Waypoints.res` at its root, call
`RunScenarioRootWorlds.cmake` from the configured `world-probe-root` with
`DB_TEST`, `WORLD_PROBE`, `GAME_DB`, `RESOURCE_DIR`, `SAVE_DIR` and
`PARTY_SAVE_MODE=ON`; `SAVE_DIR` must be a writable build-owned directory.
ARM64/QEMU also needs `TEST_EMULATOR=/usr/bin/qemu-aarch64-static`,
`QEMU_LD_PREFIX=/usr/aarch64-linux-gnu` and leak detection disabled.
Optional `START_VARIANT`/`END_VARIANT` divide the 52 roots into the nine
disjoint ranges listed above.

The first Linux x86-64 sanitizer sweep reached root 5247, where reading
the saved world exposed a
`heap-use-after-free` during process shutdown: `CTEffect::~CTEffect` accesses
`CEffect ID=1232` after the record has been freed by the database table
destructor. ARM64/QEMU reproduced this at the same root. A 5247 run without
save/load and a write-only isolation both passed; reading the world graph
alone was enough to trigger the failure, before `SerializeShared`, cache
restoration, or further ticks. Reference-count tracing showed that the
restored world failed to acquire one reference to effect 1232, while its
`CDumbUnitServer` destructor still released that pointer. The culprit was
`IRenderVisitor::SBoundEffect` in `CDumbUnitServer::attachedEffects`: the
vector was serialized as raw bytes, including a process-local `CPtr`.
It now uses an explicit object serializer with `CDBPtr<CEffect>` (DB record
ID and effect start time), so loading restores the reference. A temporary
DB-table teardown change only traded the UAF for a leak and was reverted;
the final patch changes only the bound-effect wire path and a focused test.

On the final patch, root 5247 passes save/load/resume on Linux x86-64 under
ASan/UBSan/LSan and ARM64/QEMU under ASan/UBSan (LSan disabled). Windows x64
rebuilt `Game.exe`, passed the new `NativeWorldMission5247PartySave` focused
CTest, the 52/52 extended sweep, and 151/151 non-extended tests. The full
Linux x86-64 sweep also passed 52/52 under ASan/UBSan/LSan. ARM64/QEMU
covered all 52/52 roots under ASan/UBSan (LSan disabled) in nine disjoint
numeric ranges, each root in a fresh process. Four initial ARM64 process
launches returned status 1 with empty output while the probe executable
was being relinked by a concurrent build. After the build finished, roots
4519, 5247, 5477 and 6102 passed, and their four interrupted range tails
completed. This is 52-root coverage, not one serial ARM64 CTest invocation;
do not repeat the sweep while relinking its executable. The rebuilt
non-extended Linux x86-64 and ARM64/QEMU CTest suites also passed 123/123
each under their respective sanitizer settings. Existing saves
containing a nonempty raw `attachedEffects` vector used a process-local
pointer blob and are not asserted compatible with the new structured field.
This is an own-save/own-load kernel issue for stage 2, not a comparison of
game decisions with Steam (stage 7). Steam remains available as an oracle
for specific uncertain contracts.

A clean Windows x64 native-media archive of commit `5a01d3a` was built as
`G:\SS\lab\builds\stage2-party-save-20260928-01` with 16 build jobs. Its
`Game.exe` SHA-256 is
`C69AA587E1B7F8BEF8AEE5BE82DAEB8AF5795FE8424FED68069CF1C55062D6BC`;
the archive has no `fmod.dll`. An isolated no-intro LabRun
`stage2-party-save-smoke-20260928-01` loaded the 155 portable DB tables,
rendered the main menu, and exited through the window close request without
a crash dump. This is a launch smoke, not mission or audio validation.

The game-world `deploySpots` vector (save tag 26) previously used a raw
12-byte `SWorldDeploySpot` element. Its three fields are now explicitly
encoded as a packed `SPathPlace` word and two signed 32-bit little-endian
integers, preserving the original blob shape. `NativeWorldInitProbe`
checks the exact bytes, malformed length, and field equality after saving
and restoring each party world. Root 5247 contains one actual deployment
spot; Windows x64, Linux x86-64 under ASan/UBSan/LSan, and ARM64/QEMU
under ASan/UBSan all restored it unchanged. The full Windows x64 52-root
save sweep passed with this check, as did 151/151 non-extended tests and
the rebuilt `Game.exe`. The full Linux x86-64 sweep then completed all
52/52 roots under ASan/UBSan/LSan with this check; ARM64/QEMU passed the
focused nonempty root 5247, not a repeat of the full 52-root sweep. This verifies the
world's deployment data, not post-load player placement or Steam parity.

Clean native-media Windows x64 archive `stage2-deploy-spots-20260928-01`
contains commit `38eea1b`; `Game.exe` SHA-256 is
`F8D7D7746F3FF8AAA527D004183608ECC6EBE08D68084D32D04F976A8C517928`.
No `fmod.dll` is packaged. Its no-intro LabRun opened a responsive
`Silent Storm` window and exited on `CloseMainWindow` without a dump. The
window capture was obscured by another application, so this run does not
independently prove visual menu rendering. The original `game.db`,
`AIGeometries.res`, `Buildings.res`, `Terrain.res`, and `Waypoints.res`
still match the baseline SHA-256 manifest.

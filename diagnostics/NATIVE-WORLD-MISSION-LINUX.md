# Native mission creation and post-init (stage 2)

`NativeWorldInitProbe` has optional `--mission <variant>`,
`--mission-ui-ack <variant>`, `--mission-party-ui-ack <variant>`, and
`--mission-party-shot <variant>`, and
`--mission-party-shot-save <variant>` and
`--mission-party-shot-slot <variant>`, plus
`--mission-party-explosion-save <variant> <save-file>` paths. It loads the
original `game.db` and four autoload scripts, constructs the original `CWorld`
with an RPG global game, calls `CWorld::CreateRandom` with the original
`BuildMap`, then calls `CWorld::RunPostInit`. Variant 218 exercises a small
building map with no units or attached script. Variant 810 adds two authored
units and one 1,016-byte attached Lua script; its startup also runs the
world's first-segment warm-up, vision, physics, and path-colouring jobs.

The tests are `NativeWorldMission218`, `NativeWorldMission810`,
`NativeWorldMission810UIAck`, `NativeWorldMission810PartyUIAck`, and
`NativeWorldMission810PartyShot`, and
`NativeWorldMission810PartyShotSave` and
`NativeWorldMission810PartyShotSlot` and
`NativeWorldMission810PartyExplosionSave` in both
the Windows and portable CMake builds.
Example Linux verification, from a build
configured with `S2_GAME_DB_PATH`, `S2_RESOURCE_PACKAGE_PATH`, and
`S2_SCRIPT_CORPUS_DIR`:

```sh
cmake --build build-x64 --target NativeWorldInitProbe NativeBuildingGridTests NativeHeightNetworkTests -j 16
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
  ctest --test-dir build-x64 --output-on-failure \
  -R 'NativeWorld(Mission|Init)|NativeBuildingGridTests|NativeHeightNetworkTests'
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
requires a new executor, one spent round, ten spent AP, positive damage,
and one new attack and bullet event. The selected Windows run observed
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

The probe does not create the mission UI, invoke `CMission::SaveWorld`/
`LoadWorld`, or compare dynamic AI, route, battle, and destruction decisions
against Steam x86. The variant-810 script
waits on sequence/UI actions: only the diagnostic ID-acknowledged path reaches
its later callback declaration. No real UI action or later callback invocation
is proved. The absence of Lua errors and these script
effects do not prove that every mission binding worked. Those are the next
stage-2 checks. SDL3/bgfx integration remains in stages
3-4, and no macOS result is claimed.

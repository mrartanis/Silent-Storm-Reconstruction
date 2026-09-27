# Native mission creation and post-init (stage 2)

`NativeWorldInitProbe` has optional `--mission <variant>`,
`--mission-ui-ack <variant>`, and `--mission-party-ui-ack <variant>` paths. It loads the
original `game.db` and four autoload scripts, constructs the original `CWorld`
with an RPG global game, calls `CWorld::CreateRandom` with the original
`BuildMap`, then calls `CWorld::RunPostInit`. Variant 218 exercises a small
building map with no units or attached script. Variant 810 adds two authored
units and one 1,016-byte attached Lua script; its startup also runs the
world's first-segment warm-up, vision, physics, and path-colouring jobs.

The tests are `NativeWorldMission218`, `NativeWorldMission810`,
`NativeWorldMission810UIAck`, and `NativeWorldMission810PartyUIAck` in both
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
ASan/UBSan with `halt_on_error=1`. A fresh clean x64 archive is recorded
below after the corrective source commit.
The clean native-media x64 archive from source commit `c19b144` is
`G:\SS\lab\builds\stage2-prepare-shot-20260927-01`; its `Game.exe` SHA-256
is `515177BA2C38C3C34952E2E189492E72E58CC523C80BFCDE0F392CB3B1E40207`.
It contains no `fmod.dll`. No separate live-game smoke or direct Steam
comparison is claimed for this archive.

This newly exercised path exposed two previously hidden UB cases on Linux:
`SUnitDeployData` left `bCorpseAlive` and `bCorpseEnemy` indeterminate when
adding a mercenary, and `NAI::CPath` left `bStrafePath` indeterminate before
`CExecQueue::AddPath` read it. Both now initialize to false. The original
game's tiny `RPGMerc.cpp` implementation of `CreateMerc` is included in the
portable RPG target so this is an actual game party, not a fabricated hero.

This is a headless original-world startup, **not** a full Linux game. The
party mode deploys a hero and exercises one scripted aim-only command, but
the probe does not create the mission UI, execute a full combat turn,
save/reload the resulting mission, or compare dynamic AI, route, battle,
and destruction decisions against Steam x86. The variant-810 script
waits on sequence/UI actions: only the diagnostic ID-acknowledged path reaches
its later callback declaration. No real UI action, later callback invocation,
or actual fired-shot outcome is proved. The absence of Lua errors and these two script
effects do not prove that every mission binding worked. Those are the next
stage-2 checks. SDL3/bgfx integration remains in stages
3-4, and no macOS result is claimed.

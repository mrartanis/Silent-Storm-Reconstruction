# Console config and RPG execution base on Linux (stage 2)

The original `MiscDll/Commands.cpp` now builds on Linux with a host
adapter around `PortableUserPaths`. The adapter resolves
`S2_USER_DATA_DIR`, `XDG_DATA_HOME`, or `HOME` to an absolute user-writable
directory, creates the `cfg` child, rejects symlinked directory
components/config files, and keeps `config.cfg` out of the game's
resource tree. New profile names use the same ASCII `S2U8:<UTF-8 hex>`
config representation as Windows. Untagged legacy ASCII names remain
readable; non-ASCII untagged Windows ACP names are intentionally rejected
on Linux because their codepage is not knowable there. The Linux adapter
does not claim macOS support.

`NativeConsoleConfigTests` executes the actual `RegisterVar`, `SetVar`,
`SaveConfig`, `ResetVar`, `LoadConfig`, and `ProcessCommand` path with an
isolated override root. It verifies a numeric variable and a Unicode
`game_profile` round-trip, the `S2U8` marker, and that the written file
is under the user root. A dangling-reference risk in `CVar::Get` was
removed by giving its missing-variable fallback static lifetime.

The original `RPGUnit.cpp`, `RPGItemSet.cpp`, `RPGAttackMech.cpp`,
`rpgGlobal.cpp`, and `rpgPerk.cpp` now build as
`s2_game_rpg_execution` on Linux GCC x86-64 and ARM64. The Linux build
needs the data declarations in `LSHead.h`, so the LifeStudio import and
Windows calling-convention attributes are disabled for declarations;
this does **not** port or link the proprietary FaceGen SDK. The existing
native, game-used face implementation remains a separate stage-1 path.
`NativeAttackRulesTests` executes the original corpse-push threshold,
ricochet gate, and click-of-death setup. Its output is
`push=0,0.28,1.5 ricochet=0,0,1` on restored Windows x86, Windows x64,
Linux x86-64, and ARM64. `NativePerkPointsTests` executes
the original nonnegative skill-point accounting and empty-tree random
perk behavior; all four targets yield `0,3,1,0,2` points. Neither test
substitutes for a live combat turn or a
mission's full perk tree.

An exploratory link of `NativeAILogicTests` against the available
original `s2_*.a` archives reduced distinct undefined symbols from 72
to 47. The remaining world, Lua, UI, sound/debris, and scenario
dependencies prevent a running Linux mission; no dummy implementations
or linker bypass were introduced.

Build with `cmake --build <linux-build> --target s2_game_console
s2_game_rpg_execution NativeConsoleConfigTests NativeAttackRulesTests
NativePerkPointsTests -j 16`, then run `ASAN_OPTIONS=detect_leaks=0
ctest --test-dir <linux-build> -R
'^(NativeConsoleConfigTests|NativeAttackRulesTests|NativePerkPointsTests)$'
--output-on-failure`. The full matrix uses `cmake --build <linux-build>
-j 16` and `ctest --test-dir <linux-build> --output-on-failure -j 16`
with the same ASan setting. On Windows add `--config RelWithDebInfo`
to the build and `-C RelWithDebInfo` to CTest.

The complete Windows x64 game build and CTest passed 113/113. Linux GCC
x86-64 and ARM64/QEMU built all targets and passed 85/85 each under
ASan/UBSan with `ASAN_OPTIONS=detect_leaks=0`.

A clean native-media Windows x64 archive
`G:\SS\lab\builds\stage2-console-rpg-20260926-01` was built with 16
jobs from source commit `52c270952c5ca5988edaad069c2a0947cd8f31d1`.
`Game.exe` SHA-256 is
`67B516745F3524E95BD84D1BDB153C948FE1FFE690E9E10B8485D18BBE063612`.
The archive has no FMOD DLL, and `Game.exe` imports neither `fmod.dll`
nor `FSOUND_`. In-game smoke remains unverified because the remote D3D
session cannot create a device even for an older known-good archive.

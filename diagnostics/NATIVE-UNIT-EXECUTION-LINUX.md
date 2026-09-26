# Adjacent unit execution classes on Linux (stage 2)

The original `Main/wDumbUnit.cpp`, `Main/wUnitStates.cpp`, and
`Main/wAnimation.cpp` now build as separate static targets on Linux x86-64
and ARM64: `s2_game_dumb_unit`, `s2_game_unit_states`, and
`s2_game_unit_animator`. Together with `s2_game_unit_server` and
`s2_game_ai_logic`, these are actual game translation units, not stand-ins.
They retain inventory, attack, critical-state, death, movement-animation,
and save-registration code. The Windows game continues to use the same
sources.

Portability edits normalize include paths/case; use the shared Linux
ISAAC generator for the existing random calls; use `stderr` for debug
messages on Linux; declare `EFindPathParams` consistently as `int`-backed;
and replace two MSVC-only implicit pointer conversions with explicit raw
pointer conversions. `CUnitAnimator::StandStill` takes its small position
value by value rather than a mutable reference: two callers pass a
temporary; the method only forwards the local value to the animation.
No gameplay method has been stubbed.

This is still a **compile boundary**, not a running Linux unit or AI
mission. An exploratory Linux link of the actual `NativeAILogicTests`
object with all currently compiled `s2_*.a` archives still reported 122
distinct undefined symbols. They include world/player and RPG mission
methods, UI/console registration, geometry-animation helpers, sound,
debris, and the original Lua-facing execution graph. The linker probe is
diagnostic only; no unresolved-symbol bypass is used by a passing test.
The next gate is to link and execute a live unit with the real world
dependencies, then compare game rules against the x86/Steam oracle.

Compile targets with `cmake --build <linux-build> --target
s2_game_dumb_unit s2_game_unit_states s2_game_unit_animator -j 16` on
both architectures. A full build and CTest under ASan/UBSan checks for
regressions in the already linked Linux subset, but its tests do not yet
execute these three new archives.

The full Windows x64 game build and CTest passed 109/109. Linux GCC
x86-64 and ARM64/QEMU built all targets and passed 81/81 each under
ASan/UBSan with `ASAN_OPTIONS=detect_leaks=0`.

A clean native-media Windows x64 archive
`G:\SS\lab\builds\stage2-unit-execution-20260926-01` was built with
16 jobs from source commit
`9328b761dfb88ec77a2697935233051231733fea`. `Game.exe` SHA-256 is
`8622B85AB3DF53D21908A74C0DDBA379F0691E360A14CAFF6E210E2F793B1F29`.
The archive has no FMOD DLL, and `Game.exe` imports neither `fmod.dll`
nor `FSOUND_`. In-game smoke remains unverified because the remote D3D
session cannot create a device even for an older known-good archive.

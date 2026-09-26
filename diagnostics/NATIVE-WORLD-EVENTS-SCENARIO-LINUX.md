# World events and scenario scripting on Linux (stage 2)

The game-used `wAckBase.cpp`, `wAck.cpp`, `wPocket.cpp`,
`wUnitGroup.cpp`, `wBullet.cpp`, `wGrenade.cpp`, `wKnife.cpp`, and
`wRocket.cpp`, and `wExplTracker.cpp` now compile into `s2_game_world_events` on Linux
GCC x86-64/ARM64. The scenario and Lua bridge units
`scScenarioTracker.cpp`, `scFlowChartItems.cpp`, `scFlowChart.cpp`,
`scCommands.cpp`, `scriptCommon.cpp`, `A5Script.cpp`, and
`scriptPtr.cpp` compile into `s2_game_scenario_scripts`.

The edits make Windows PCH prerequisites explicit, normalize include
case and separators, use the existing game RNG rather than libc's
`random` on Linux, and use a 32-bit monotonic millisecond seed in the
scenario fallback (matching the width of Windows `GetTickCount`).
GCC-only template and temporary-object errors were corrected without
changing the intended rules. The `wGrenade.h` dependency on the game's
`STime` now names `A5Time.h` explicitly.

The diagnostic link of the real `NativeAILogicTests` object with all
currently available game archives still fails on 41 distinct unresolved
symbols, down from 66 before this package. The remaining calls include
world buildings/terrain, unit and render objects, a rocket server, and
script UI registration. No dummy implementations or unresolved-symbol
linker bypasses were used. The Linux game and live AI are not yet
executable.

The source file `wRocket.cpp` contained a CP1251 comment. It was
converted to UTF-8 without changing executable code, and its `STime`
include now names `A5Time.h`. `scriptUI.cpp` is not in the portable core;
it binds Lua to window/graphics UI
objects and must be handled at the UI integration boundary without
claiming stages 3–4 complete.

The Windows x64 `RelWithDebInfo` game builds and passes 116/116 CTest
cases. Linux GCC x86-64 and ARM64/QEMU build all declared targets and
pass 90/90 tests each under ASan/UBSan. However, this package has no
new focused runtime test of ack decisions, explosions,
scenario branching, or live Lua calls. The existing mission-script
corpus test checks resource bytes, not execution of these new units.
Behavioral parity with the Steam EXE and a headless executable world
remain open. The clean archive is recorded below after verification.

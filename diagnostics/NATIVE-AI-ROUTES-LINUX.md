# AI route and control compilation on Linux (stage 2)

The original game-used `aiCommander`, `aiRoute`, `aiRouteLogic`,
`aiRouteMisc`, `aiReaction`, `aiReactions`, `aiNearestPosition`,
`aiTaskCommand`, `aistate`, `aiUnit`, `aiPlayer`, and `aiEvent` translation
units now form the `s2_game_ai_routes` static archive on Linux GCC
x86-64 and ARM64. Source changes normalize case-sensitive includes,
make enum forward declarations valid C++17, and use the existing
portable game RNG and stderr diagnostics on Linux while preserving the
Windows RNG and debug-output paths. The algorithms are not replaced.

This remains a **compile boundary**, not a live Linux AI turn. A
diagnostic link of the actual `NativeAILogicTests` object against the
available original game archives still fails on 148 distinct unresolved
symbols, down from 191 after adding the route support group. The
remaining dependencies include the map and mission runtime, object
actions, Lua/scenario hooks, and render/sound world objects. No stub
implementations or unresolved-symbol linker allowance were added.

Build with `cmake --build <linux-build> --target s2_game_ai_routes -j 8`.
The archive build only verifies the source translation units; full AI
behavior and Steam parity require an executable game world and runtime
tests in subsequent packages.
Windows x64 `RelWithDebInfo` rebuilt `Game.exe` and passed 113/113
CTest cases; Linux GCC x86-64 and ARM64/QEMU passed 85/85 each under
ASan/UBSan with `ASAN_OPTIONS=detect_leaks=0`.

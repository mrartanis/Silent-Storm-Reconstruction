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

A clean native-media Windows x64 archive
`G:\SS\lab\builds\stage2-ai-routes-20260926-01` was built with 16
jobs from commit `38e7b8f5d45b91b0863b1506d66b873a0ef90139`.
`Game.exe` SHA-256 is
`ECAA195AE3E7D3F236B76C0F8FEC5CEDCC45A6B32989358E1BA4559A4433B4E7`.
The archive contains no FMOD DLL and the executable has no `fmod.dll`
or `FSOUND_` imports. In-game smoke is still unverified: the current
remote D3D session fails to create a device even for an older known-good
archive.

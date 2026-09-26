# World coordinator compile boundary on Linux (stage 2)

The original `Main/wMain.cpp`, `wMainMoves.cpp`, `wMainTrace.cpp`, and
`wUICommands.cpp` are compiled together as `s2_game_world` on Linux GCC
x86-64 and ARM64. The port keeps the Windows random generator and debug
output paths while using the existing portable game generator and stderr
on Linux. The voxel-renderer's Linux-only scalar row transform is the
same matrix operation as the Windows vector aliases, which are absent
from the portable `SHMatrix` layout. No renderer is being substituted.

This is a **compile boundary**, not a playable Linux world. A diagnostic
link of the real `NativeAILogicTests` object against the available game
archives resolves the prior `CWorld` and UI-command gaps but still fails
on 120 distinct unresolved symbols. Bringing in the world coordinator
expands the dependency graph: mission objects, scenario/Lua bindings,
sound/debris, terrain, and UI/render classes remain outside the complete
Linux game link. We have not added dummy implementations or accepted
unresolved symbols to produce an executable.

Build the boundary with `cmake --build <linux-build> --target
s2_game_world -j 16`. This only proves the original translation units
compile and form a static archive; runtime behavior and the full game
must be tested after the remaining game-used dependencies are linked.
The full Linux GCC x86-64 and ARM64/QEMU builds and CTest runs passed
85/85 each under ASan/UBSan (`ASAN_OPTIONS=detect_leaks=0`).
Windows x64 `RelWithDebInfo` rebuilt `Game.exe` and passed 113/113
CTest cases. A diagnostic `Debug` CTest run was stopped after legacy
assertion dialogs appeared in resource tests; it is not counted as a
passing run.

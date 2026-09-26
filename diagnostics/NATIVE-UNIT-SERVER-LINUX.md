# Unit server compilation on Linux (stage 2)

The original, complete `Main/wUnitServer.cpp` now builds as
`s2_game_unit_server` with GCC on Linux x86-64 and ARM64. The Windows game
continues to compile the same source. The port normalizes include case and
separators, makes several existing enum declarations explicitly `int`-backed,
uses the shared Linux ISAAC generator at the three random-call sites, and
maps the MSVC debug trap to a compiler trap. No unit-server method was
replaced with a test double.

This is a **compile boundary**, not a linked or running Linux unit server.
An exploratory link with the real `CAILogic` still requires original
`CDumbUnitServer`, `CUnitState`, `CWorld`, `CPlayer`, `CCannon`, unit
animation, RPG/path, and Lua/world methods. The server archive's existence
does not demonstrate AI decisions, movement, or combat in a Linux mission.
No unresolved-symbol linker exception or fake implementation is counted
as passing that gate. The next meaningful port is the adjacent dumb-unit,
state, and world graph, followed by a runtime test with a live unit.

Reproduce compilation with `cmake --build <linux-build> --target
s2_game_unit_server -j 16` on each architecture. The full Linux build and
CTest matrix uses `cmake --build <linux-build> -j 16` followed by
`ASAN_OPTIONS=detect_leaks=0 ctest --test-dir <linux-build>
--output-on-failure -j 16`. The Linux x86-64 suite passed 81/81 under
ASan/UBSan; ARM64/QEMU passed 81/81 under the same sanitizers. The full
Windows x64 game build and CTest passed 109/109. These suites do not
execute `CUnitServer` itself.

The clean-archive provenance is recorded below once packaging finishes.

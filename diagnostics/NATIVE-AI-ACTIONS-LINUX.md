# AI action and RPG combat compilation on Linux (stage 2)

The original game-used AI actions, firearms/weapon decisions, action-place
source, combat log, interval tracing, jobs, multi-moves, move transitions,
and path logic now compile into `s2_game_ai_actions` on Linux GCC x86-64
and ARM64. The original `RPGToHit.cpp` and `RPGCritical.cpp` compile into
`s2_game_rpg_combat`. The changes select the portable existing headers,
normalize case-sensitive includes, preserve 32-bit enum representations,
and replace Windows-only diagnostics on Linux. The invalid MSVC token
pasting around AI weapon method names was replaced by standard C++ syntax
with the same expansion.

`NativeAIIntervalTests` executes the original interval-trace code for
unsorted model intersections and an incoming terrain ray. It passes on
Linux x86-64 and ARM64/QEMU under ASan/UBSan. This is a narrow regression, not proof
of complete AI decisions or RPG combat parity.

The Linux game still does **not** link. A diagnostic link of the real
`NativeAILogicTests` object against all available original game archives
fails on 154 distinct unresolved symbols, down from 232 before this
group. Remaining dependencies include specialized AI actions/jobs,
world objects, animation, RPG, timers, Lua, and scenario hooks. There
are no dummy implementations or unresolved-symbol linker bypasses.

The Windows x64 `RelWithDebInfo` build and CTest passed 115/115 cases;
Linux GCC x86-64 and ARM64 built all declared targets and passed 88/88
tests each under ASan/UBSan. An in-game smoke for this batch is not claimed: the
current remote Windows D3D session cannot create a device even for an
older known-good archive.

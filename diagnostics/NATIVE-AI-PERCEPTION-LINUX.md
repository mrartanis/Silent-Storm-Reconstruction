# AI perception and combat compilation on Linux (stage 2)

The original game-used threat tracker, unit state, AI inventory and
miscellaneous world glue, combat logic, defence/assassin/fear/guard
reactions, action/move helpers, ray and voxel tracers, and stability
logic now compile together as `s2_game_ai_perception` on Linux GCC
x86-64 and ARM64. The source changes normalize case-sensitive include
paths, make previously implicit PCH dependencies explicit, and select
the existing portable game RNG and stderr diagnostics on Linux. Enum
declarations and definitions use explicit `int` bases where GCC
requires them; this preserves the game's 32-bit enum storage.

`NativeAITraceSphereTests` executes the original `CTracer` projection
and sphere-distance path in two principal axis directions. It produces
`1,0,1` on restored Windows x86, Windows x64, and Linux x86-64/ARM64
under ASan/UBSan. The x86 run is a reference check, not a product build.
This is a focused geometry regression, not a combat or visibility
parity test.

The full Linux game still does **not** link. A diagnostic link of the
real `NativeAILogicTests` object against all available original game
archives fails on 232 distinct unresolved symbols. Adding this large
perception/combat group resolves the earlier direct tracker, tracer,
and reaction methods but exposes more dependencies in actions,
visibility, RPG combat, world objects, and Lua/scenario hooks. No
dummy implementation or unresolved-symbol linker bypass is used.

Build with `cmake --build <linux-build> --target
s2_game_ai_perception NativeAITraceSphereTests -j 8`; run the focused
test with `ASAN_OPTIONS=detect_leaks=0 ctest --test-dir <linux-build>
-R '^NativeAITraceSphereTests$' --output-on-failure`.
The complete Windows x64 `RelWithDebInfo` build passed 115/115 CTest
cases, and Linux GCC x86-64 and ARM64/QEMU passed 87/87 each with
`ASAN_OPTIONS=detect_leaks=0`.

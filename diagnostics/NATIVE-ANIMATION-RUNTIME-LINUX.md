# Original animation runtime and height sampler on Linux (stage 2)

The original `GAnimBase.cpp`, `GAnimFormat.cpp`, `GAnimation.cpp`,
`GSkeleton.cpp`, `GAnimTerrain.cpp`, `GAnimPath.cpp`, and
`GAnimParticles.cpp` now compile together as `s2_game_animation_runtime`
on Linux GCC x86-64 and ARM64. `aiHeight.cpp` builds as
`s2_game_ai_height`. These are the game's implementations used by the
unit animator, including skeleton poses, path interpolation, terrain
placement, and corpse/particle motion. Their Windows translation units
are unchanged apart from equivalent include spelling, pointer-neutral
diagnostics, and selection of the same ISAAC generator via the Linux
symbol.

`NativeAnimationPathTests` executes the actual `CPathInterpolator` from
`GAnimPath.cpp`: it checks endpoint clamping, midpoint interpolation,
direction, and the smoothed distance of a three-point bend. Its output
is `straight=2.000000 bend=1.695598 points=65` on the restored Windows
x86 oracle, Windows x64, and Linux GCC x86-64/ARM64 under ASan/UBSan.

This is still not the full Linux animation or game loop. A diagnostic
link of `NativeAILogicTests` against all current `s2_*.a` archives
reduced the distinct undefined symbols from 122 to 72; among the
remaining requirements are RPG mission/world methods, command console,
UI, script bridge, and sound/debris. That diagnostic binary did not
link or run. No fake class or unresolved-symbol exception was accepted
as a runtime pass.

Build with `cmake --build <linux-build> --target
s2_game_animation_runtime s2_game_ai_height NativeAnimationPathTests
-j 16`; run `ASAN_OPTIONS=detect_leaks=0 ctest --test-dir <linux-build>
-R '^NativeAnimationPathTests$' --output-on-failure`. The full Linux
matrix uses `cmake --build <linux-build> -j 16` and `ctest --test-dir
<linux-build> --output-on-failure -j 16` with the same ASan setting.
On Windows, build/test `NativeAnimationPathTests` in `RelWithDebInfo`
and run the full CTest suite with `-C RelWithDebInfo`.

The complete Windows x64 build/CTest passed 110/110; Linux GCC x86-64
and ARM64/QEMU built all targets and passed 82/82 each under ASan/UBSan.
The newly ported targets other than `CPathInterpolator` are still
compile-checked only. A clean archive's provenance follows below.

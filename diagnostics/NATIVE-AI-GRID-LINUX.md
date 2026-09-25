# Original path-network compile boundary

Stage 2 now compiles the game's original `Main/aiGrid.cpp` translation unit
as `s2_game_ai_grid` on Linux GCC x86-64/ARM64 and Clang x86-64. This is a
compile gate, not yet a runnable Linux `CPathNetwork` or mission-routing
claim. The source is included in the complete headless build so later
portable-header changes cannot silently break it.

The compile port replaced the Windows precompiled-header dependency with
the existing headless platform/structure/geometry headers, normalized
game-used include paths and case, made the original object-refcount macro
valid for template classes, and made dependent iterator/base names explicit
for GCC/Clang. Forward-declared enums used by the world interface now have
fixed `int` underlying types (their original runtime size); `aiGrid.cpp`'s
debug output uses portable formatting on Linux. The transient `SMove`
non-Windows representation and `STempArray<T>` macro route are documented
in `NATIVE-HEIGHT-LAYERS-LINUX.md`.

Clang exposed a one-before-array pointer in the original `CPool<T>` block
cursor. `Pool.h` now uses a next-free pointer and a legal one-past-end
pointer; `NativePoolTests` allocates across three blocks, iterates back
over each boundary, clears, and reuses the pool. This code is used by the
AI renderer and other game systems and is not a serialized layout.

A subsequent link probe pulled in the original position/lock helpers,
pass/move calculators, zone colourer, fast AI rasterizer, object/collider
and physics-collider implementations, super-collider, volume container,
and game console stream. These compile on GCC Linux x86-64/ARM64 and
Clang x86-64 as `s2_game_ai_position`, `s2_game_ai_locker`,
`s2_game_ai_pass_jobs`, `s2_game_ai_calculators`, `s2_game_ai_render`,
`s2_game_ai_colourer`, `s2_game_ai_collision`, and `s2_game_ai_log`.
`NativeAILogTests` executes the original log stream's AI/system line,
number, boolean, colour, and newline path on all three Linux targets.
The cross-architecture port also made the map-direction deltas explicitly
`signed char` (plain `char` is unsigned on ARM64), used scalar matrix rows
instead of Windows-only `CVec4` aliases in the fast renderer, normalized
include paths/case, and made the log's conversions portable.

In `aiLocker.cpp`, the concrete `CUnitServer` cast was replaced with its
`CUnit` game interface; the original `IsWearingPK()` test is exactly
`IsValid(GetWearingDBPK())`. In `aiGrid.cpp`, the concrete `CMine` cast
in an otherwise no-op branch was replaced with the `IMine` interface.
The Windows x64 `Game` target rebuilt, and CTest passed 91/91 after
those common-source edits.

The latest full link probe fails on only the original
`NWorld::CheckItemsBreakGlass(NAI::SSourceInfo const*)` from `wOSBase.cpp`.
This is real game behaviour: contact with breakable glass can advance its
destroy stage and let the moving sphere pass. A fake always-solid stub
would change grenade and movement behaviour, so no stub is linked.
Accordingly the height-layer test still discards the `ComputeLayers`
path-network branch as described in `NATIVE-HEIGHT-LAYERS-LINUX.md`.
Full execution of that branch, with the original glass behaviour, remains
required before claiming building-aware floor heights or Linux routing.

On the authorized Linux host, after syncing the repository source:

```sh
cmake -S /tmp/s2-matrix-links.zCkO0O/src -B /tmp/s2-matrix-links.zCkO0O/build-x64 -DS2_GAME_DB_PATH=/tmp/s2-matrix-links.zCkO0O/game.db
cmake --build /tmp/s2-matrix-links.zCkO0O/build-x64 --target s2_game_ai_grid s2_game_ai_position s2_game_ai_locker s2_game_ai_pass_jobs s2_game_ai_calculators s2_game_ai_render s2_game_ai_colourer s2_game_ai_collision s2_game_ai_log NativePoolTests NativeAILogTests -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-x64 -R 'NativePoolTests|NativeAILogTests' --output-on-failure
cmake -S /tmp/s2-matrix-links.zCkO0O/src -B /tmp/s2-matrix-links.zCkO0O/build-arm64 -DS2_GAME_DB_PATH=/tmp/s2-matrix-links.zCkO0O/game.db
cmake --build /tmp/s2-matrix-links.zCkO0O/build-arm64 --target s2_game_ai_grid s2_game_ai_position s2_game_ai_locker s2_game_ai_pass_jobs s2_game_ai_calculators s2_game_ai_render s2_game_ai_colourer s2_game_ai_collision s2_game_ai_log NativePoolTests NativeAILogTests -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-arm64 -R 'NativePoolTests|NativeAILogTests' --output-on-failure
export CPLUS_INCLUDE_PATH=/usr/include/c++/11:/usr/include/x86_64-linux-gnu/c++/11:/usr/include/c++/11/backward
export LIBRARY_PATH=/usr/lib/gcc/x86_64-linux-gnu/11
cmake -S /tmp/s2-matrix-links.zCkO0O/src -B /tmp/s2-matrix-links.zCkO0O/build-clang-release -DS2_GAME_DB_PATH=/tmp/s2-matrix-links.zCkO0O/game.db
cmake --build /tmp/s2-matrix-links.zCkO0O/build-clang-release --target s2_game_ai_grid s2_game_ai_position s2_game_ai_locker s2_game_ai_pass_jobs s2_game_ai_calculators s2_game_ai_render s2_game_ai_colourer s2_game_ai_collision s2_game_ai_log NativePoolTests NativeAILogTests -j 16
ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-clang-release -R 'NativePoolTests|NativeAILogTests' --output-on-failure
```

The deliberately failing complete-link probe, from `build-x64` after all
targets above are built, is:

```sh
cd /tmp/s2-matrix-links.zCkO0O/build-x64
g++ -fsanitize=address,undefined -Wl,--gc-sections \
  CMakeFiles/NativeHeightLayersTests.dir/diagnostics/NativeHeightLayersTests.cpp.o \
  -o /tmp/s2-ai-grid-link-probe -Wl,--start-group \
  libs2_game_height_layers.a libs2_game_ai_position.a libs2_game_ai_locker.a \
  libs2_game_ai_pass_jobs.a libs2_game_ai_calculators.a libs2_game_ai_render.a \
  libs2_game_ai_colourer.a libs2_game_ai_collision.a libs2_game_ai_log.a \
  libs2_game_ai_grid.a libs2_game_terrain_info.a libs2_game_dg.a \
  libs2_game_structure.a libs2_game_streams.a libs2_game_objects.a \
  libs2_portable_structure.a libs2_game_beta_spline.a libs2_game_transform.a \
  libs2_game_misc_runtime.a -Wl,--end-group
```

It must not be treated as a passing test: the remaining undefined symbol is
`NWorld::CheckItemsBreakGlass(NAI::SSourceInfo const*)`. In the Windows game
it is defined in `Main/wOSBase.cpp`. Extracting or linking that genuine
world behaviour is the next dependency before an executable Linux route
test can be added.

Windows `NativePoolTests` is built with the original `Main` include
environment; the full Windows x64 `Game` target and CTest suite remain the
regression gate for the shared headers. The Windows x64 game rebuilt and
91/91 CTest passed; the x86 `NativePoolTests` also passed. Linux GCC x86-64
and ARM64 sanitizer suites and Clang x86-64 each passed 64/64 before the
new `NativeAILogTests`; that test passed individually on all three. A full
rebuild/regression after adding it passed 65/65 on GCC x86-64, GCC ARM64,
and Clang x86-64. As
elsewhere, the recovered x86 build is a supplemental implementation
comparator, not Steam itself.

Clean native-media x64 archive
`G:\SS\lab\builds\stage2-ai-grid-compile-20260925-01` was produced
from source commit `79cc6e9`. Fresh linked-resource run
`G:\SS\lab\runs\stage2-ai-grid-compile-clean-01` had no `fmod.dll`,
loaded the existing `DB_OLD` slot to `LOAD-SLOT-DONE`, accepted `quit`,
and exited without a crash dump. Its `Game.exe` SHA-256 matched the archive:
`C70F04B1AFA3A7A985FD37733778B8E426C0DA20DD8C9015B75AA8CEAE9C3A4B`.
This checks the Windows x64 game after common-header edits, not Linux
`CPathNetwork` execution.

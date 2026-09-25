# Original path-network height execution boundary

Stage 2 compiles the game's original `Main/aiGrid.cpp` translation unit
as `s2_game_ai_grid` on Linux GCC x86-64/ARM64 and Clang x86-64. It now also
links and runs a synthetic `CPathNetwork` tile through the original
`CHeightLayers::ComputeLayers` and beta-spline path in
`NativeHeightNetworkTests`. All three Linux targets and Windows x86/x64 print
`height-network maximum=0.27720881 field=CC09769B5563B4C2 tiles=16`
for this case. The hash covers every float in the post-spline 5x5 field,
and both the maximum and hash are assertions in the test. This verifies an
executing path-network/height-layer branch, not a loaded mission or AI
route parity claim.

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

The last missing link, `NWorld::CheckItemsBreakGlass`, is now supplied by
compiling the original `Main/wOSBase.cpp` as `s2_game_world_object`. This
preserves the game's breakable-glass contact behaviour; there is no
always-solid stub. Porting this translation unit required correct case in
its dependent headers, explicit `typename` for world/TBS template types,
fixed underlying types for forward-declared enums, and complete
`CFileRequest` at its inline caller. GCC x86-64/ARM64 and Clang x86-64
compile it. The executable height-network test links it with the game AI,
collider, DG, transform, terrain and serializer libraries.

`s2_game_world_object` uses `-fno-sanitize=vptr` while keeping ASan and the
other UBSan checks: vptr metadata for unrelated world-object methods
retains RTTI for the not-yet-linked whole renderer/world graph even when
those methods are section-garbage-collected. This exception is temporary
and must be removed when that graph is linked. The separate height-layer
target has the older, analogous exception; see
`NATIVE-HEIGHT-LAYERS-LINUX.md`.

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

For the newly linked game-network branch, run on each configured build
(`build-x64`, `build-arm64`, `build-clang-release`):

```sh
cmake --build /tmp/s2-matrix-links.zCkO0O/build-x64 --target NativeHeightNetworkTests -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-x64 -R NativeHeightNetworkTests --output-on-failure -V
cmake --build /tmp/s2-matrix-links.zCkO0O/build-arm64 --target NativeHeightNetworkTests -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-arm64 -R NativeHeightNetworkTests --output-on-failure -V
cmake --build /tmp/s2-matrix-links.zCkO0O/build-clang-release --target NativeHeightNetworkTests -j 16
ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-clang-release -R NativeHeightNetworkTests --output-on-failure -V
```

The `NativeHeightNetworkTests` CMake target groups the original static
libraries explicitly because AI, world, serializer and DG objects have
circular link dependencies; it does not provide alternate implementations
of game methods. The test sets one tile height in a 4x4 layer, calls the
real `ComputeLayers(4,4,nullptr,network)`, and checks a separate nonzero
floor field after smoothing. It does not build a map, route a unit, or
compare a campaign decision with Steam. The previous failing link probe
is superseded by this executable test.

Windows `NativeHeightNetworkTests` links the full `Main` library. The
Windows x64 game rebuilt and the complete CTest suite passed 92/92.
Linux GCC x86-64/ARM64 sanitizer suites and Clang x86-64 passed 66/66.
The recovered Windows x86 build produced the same complete-field hash;
it is a supplemental implementation comparator, not Steam itself.
With the copied VS CMake on Windows, run:

```powershell
$cmake = 'G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin'
$env:UCRTContentRoot = 'C:\Program Files (x86)\Windows Kits\10\'
& "$cmake\cmake.exe" --build G:\SS\lab\build-x64-stage2 --config RelWithDebInfo --target NativeHeightNetworkTests --parallel 16
& "$cmake\ctest.exe" --test-dir G:\SS\lab\build-x64-stage2 -C RelWithDebInfo -R NativeHeightNetworkTests --output-on-failure -V
& "$cmake\cmake.exe" --build G:\SS\lab\build-x86-transform --config RelWithDebInfo --target NativeHeightNetworkTests --parallel 16
& "$cmake\ctest.exe" --test-dir G:\SS\lab\build-x86-transform -C RelWithDebInfo -R NativeHeightNetworkTests --output-on-failure -V
```

Clean native-media x64 archive
`G:\SS\lab\builds\stage2-ai-grid-compile-20260925-01` was produced
from source commit `79cc6e9`. Fresh linked-resource run
`G:\SS\lab\runs\stage2-ai-grid-compile-clean-01` had no `fmod.dll`,
loaded the existing `DB_OLD` slot to `LOAD-SLOT-DONE`, accepted `quit`,
and exited without a crash dump. Its `Game.exe` SHA-256 matched the archive:
`C70F04B1AFA3A7A985FD37733778B8E426C0DA20DD8C9015B75AA8CEAE9C3A4B`.
This checks the Windows x64 game after common-header edits, not Linux
`CPathNetwork` execution.

After the direct AI dependencies were added, clean native-media x64 archive
`G:\SS\lab\builds\stage2-ai-dependencies-20260925-01` was built from
source commit `8a47e2d`. A fresh linked-resource run
`G:\SS\lab\runs\stage2-ai-dependencies-clean-01` loaded the existing
`DB_OLD` save to `LOAD-SLOT-DONE`, accepted `quit`, and exited with no
crash dump or `fmod.dll`. Archive and run `Game.exe` SHA-256 both equal
`4757108D1A583FC855CCCAB081BAA16A7A43CD18969FFE6818823899838C38FC`.
This is again a Windows x64 smoke test; it does not prove execution of
the Linux AI path network.

After the linked height-network test, clean native-media x64 archive
`G:\SS\lab\builds\stage2-ai-height-network-20260925-01` was built from
source commit `332b162`. Fresh linked-resource run
`G:\SS\lab\runs\stage2-ai-height-network-clean-01` loaded `DB_OLD` to
`LOAD-SLOT-DONE`, accepted `quit`, and exited without a dump or `fmod.dll`.
Archive and run `Game.exe` SHA-256 both equal
`4DC44CE1316ACAEC0D1384A3B6A17EBF86A9CB96A92941A28D0BAD7594754226`.
This checks the Windows x64 game after world-header changes. The Linux
`ComputeLayers` evidence is the separately linked and executed
`NativeHeightNetworkTests`, not the Windows smoke run.

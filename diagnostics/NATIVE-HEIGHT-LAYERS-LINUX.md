# Height-layer cache boundary on Linux

Stage 2 compiles the original `Main/wHeightLayers.cpp` on Linux x86-64,
ARM64, and Clang x86-64. `NativeHeightLayersTests` links and runs its real
`CHeightLayers` floor-cache methods: the shared terrain height field, deep
copy when creating floor 2, mapping requested floor 3 to floor 2, and a
`CStructureSaver` round-trip of both height fields and the floor remap.
Expected output: `height terrain=6.25 floor=9.50 mapped=2`.
Windows x64 CTest passes 90/90 and the Linux x86-64, ARM64 sanitizer, and
Clang x86-64 suites each pass 63/63. The recovered Windows x86 build
prints the same `6.25/9.50/2` values; it is a supplemental implementation
comparison, not a substitute for a Steam runtime oracle.

The cache test above is not the full `ComputeLayers` path. A second test,
`NativeHeightNetworkTests`, now creates a real `CPathNetwork`, sets a tile
height, runs `ComputeLayers`, and checks the separate floor field after
beta-spline smoothing. Windows x64 and Linux GCC x86-64/ARM64 and Clang
x86-64 and recovered Windows x86 all report maximum `0.27720881` and
post-spline 5x5 field hash `CC09769B5563B4C2`; see
`NATIVE-AI-GRID-LINUX.md`. This is a synthetic map fragment, not proof
that an original mission builds the same layers or routes units correctly.
`-fno-sanitize=vptr` is still applied to the height translation unit: its
other methods' vptr metadata retain unrelated world RTTI when section-GC
prunes the still-unlinked full world graph. ASan and the other UBSan checks
remain enabled. Remove this exception when the whole graph is linked.

`aiPosition.h` now represents the transient `SMove` pair as `dest/type` on
non-Windows compilers; MSVC keeps its original anonymous-union aliases.
`aiGrid.h` supplies the same object-refcount methods explicitly for the
`STempArray<T>` template on non-Windows compilers because the MSVC macro's
template destructor syntax is rejected by GCC/Clang. These header edits
do not alter serialized tile or path-place wire formats.

On the authorized Linux host:

```sh
cmake -S /tmp/s2-matrix-links.zCkO0O/src -B /tmp/s2-matrix-links.zCkO0O/build-x64 -DS2_GAME_DB_PATH=/tmp/s2-matrix-links.zCkO0O/game.db
cmake --build /tmp/s2-matrix-links.zCkO0O/build-x64 -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-x64 --output-on-failure -j 8
cmake -S /tmp/s2-matrix-links.zCkO0O/src -B /tmp/s2-matrix-links.zCkO0O/build-arm64 -DS2_GAME_DB_PATH=/tmp/s2-matrix-links.zCkO0O/game.db
cmake --build /tmp/s2-matrix-links.zCkO0O/build-arm64 -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-arm64 --output-on-failure -j 8
export CPLUS_INCLUDE_PATH=/usr/include/c++/11:/usr/include/x86_64-linux-gnu/c++/11:/usr/include/c++/11/backward
export LIBRARY_PATH=/usr/lib/gcc/x86_64-linux-gnu/11
cmake -S /tmp/s2-matrix-links.zCkO0O/src -B /tmp/s2-matrix-links.zCkO0O/build-clang-release -DS2_GAME_DB_PATH=/tmp/s2-matrix-links.zCkO0O/game.db
cmake --build /tmp/s2-matrix-links.zCkO0O/build-clang-release -j 16
ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-clang-release --output-on-failure -j 8
```

On Windows, `NativeHeightLayersTests` links the full `Main` library. CTest
sets the native media DLL search path for x64 and the baseline stub path for
x86. The copied VS toolchain uses `vcvars64.bat` or `vcvars32.bat`, then
`cmake --build ... --config Release --target NativeHeightLayersTests --parallel 16 -- /p:UseEnv=true`.

The next substantive dependency is original mission-map construction and
route behaviour on Linux, then comparison of game-used floor heights and
decisions with the recovered x86 implementation and, where observable,
Steam. This synthetic tile test does not establish mission parity.

Clean native-media x64 archive
`G:\SS\lab\builds\stage2-height-cache-20260925-01` came from source commit
`d4b11b7`. Fresh linked-resource run
`G:\SS\lab\runs\stage2-height-cache-clean-01` contained no `fmod.dll`,
loaded the existing `DB_OLD` mission slot to `LOAD-SLOT-DONE`, accepted
`quit`, and exited without a crash dump. Its `Game.exe` SHA-256 matches the
archive (`5F28E63D9550460CF89EB2CA1A8EB229C069834ADEB73D3350FBB455FC0B7F22`).
This Windows smoke test does not exercise the Linux path-network branch.

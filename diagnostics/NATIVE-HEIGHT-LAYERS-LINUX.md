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

This is not yet the full `ComputeLayers` path. That method includes
`NAI::CPathNetwork` tile rasterization and spline smoothing. The original
`aiGrid.cpp` is still Windows-only: a direct Linux build reaches its
`Main/StdAfx.h` dependency on `vcruntime_typeinfo.h`. The height-cache
test links only methods it exercises by compiling the original translation
unit with function/data sections and dropping unused sections at link time.
`-fno-sanitize=vptr` is applied to that translation unit because UBSan's
vptr metadata for the unlinked `CPathNetwork` branch otherwise retains its
unresolved RTTI even when the branch is discarded. ASan and the remaining
UBSan checks still run on GCC x86-64 and ARM64. This exception must be
removed when the real path-network implementation is linked.

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

The next substantive dependency is a portable, linked `CPathNetwork`, then
an integration test that supplies real path-network tiles to `ComputeLayers`
and compares the resulting floor heights with the recovered x86 reference
and, where observable, Steam. A cache-only test cannot establish mission
routing or building-aware height parity.

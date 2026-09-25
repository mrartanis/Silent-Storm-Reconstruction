# Native game transform mathematics on Linux

Stage 2 now compiles the game's original `Main/Transform.cpp` as
`s2_game_transform` on Linux x86-64 and ARM64. The Windows game still builds
the same source through `Main`. This covers camera projection, matrix stacks,
inversion, bounds and cover rectangles without claiming a Linux renderer or
game loop.

The port replaces the MSVC-only `SHMatrix` vector aliases used in this file
with explicit numeric rows, and `ZeroMemory` with `std::memset`. The projection
shift retains the original row arithmetic. Dependent base members are named
explicitly in the two matrix-stack templates; two quaternion stack calls now
pass the required matrix pointer. No transform algorithm was intentionally
changed.

`NativeTransformTests` checks translation and inverse, rotation and inverse,
bound transformation and radius, projective shift, and a finite projected
cover rectangle. The test pins representative rotation and cover values.
Windows x86 (supplemental restored-source oracle), Windows x64, Linux GCC
x86-64, Linux GCC ARM64/QEMU, and Linux Clang 14 x86-64 produced exactly the
same printed single-precision values:

```
rotated=2.03938246,-0.988380671,5.09628296 radius2=14 cover=-0.0956955254,0.408195525,-0.570194125,-0.0365372747
```

The recovered x86 game is useful for this focused comparison; the original
Steam executable remains the gameplay reference. This deterministic unit
test is not a camera/frame comparison against Steam.

On the current Windows host:

```powershell
$env:UCRTContentRoot='C:\Program Files (x86)\Windows Kits\10\'
$cmake='G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
& $cmake --build 'G:\SS\lab\build-x64-stage2' --config RelWithDebInfo --target NativeTransformTests --parallel 12
& 'G:\SS\lab\build-x64-stage2\RelWithDebInfo\NativeTransformTests.exe'
```

On the authorized Linux host, use the configured portable-core build trees:

```sh
cmake --build /tmp/s2-matrix-links.zCkO0O/build-x64 --target NativeTransformTests -j 16
ASAN_OPTIONS=detect_leaks=0 /tmp/s2-matrix-links.zCkO0O/build-x64/NativeTransformTests
cmake --build /tmp/s2-matrix-links.zCkO0O/build-arm64 --target NativeTransformTests -j 16
ASAN_OPTIONS=detect_leaks=0 qemu-aarch64-static -L /usr/aarch64-linux-gnu /tmp/s2-matrix-links.zCkO0O/build-arm64/NativeTransformTests
export CPLUS_INCLUDE_PATH=/usr/include/c++/11:/usr/include/x86_64-linux-gnu/c++/11
export LIBRARY_PATH=/usr/lib/gcc/x86_64-linux-gnu/11
cmake --build /tmp/s2-matrix-links.zCkO0O/build-clang-release --target NativeTransformTests -j 16
/tmp/s2-matrix-links.zCkO0O/build-clang-release/NativeTransformTests
```

After adding the pinned-value assertions, full CTest passed: Windows
x64 86/86, Linux GCC x86-64 56/56, Linux GCC ARM64/QEMU 56/56, and Linux
Clang x86-64 release 56/56. The GCC Linux suites ran under ASan/UBSan
(`ASAN_OPTIONS=detect_leaks=0` for QEMU). Full game camera behavior, graphics parity, Linux
mission loading, and the remaining `Main/` world classes are still open.

Clean Windows x64 archive
`G:\SS\lab\builds\stage2-native-transform-20260925-01` was produced from
source commit `0d06a98` with native media. A fresh linked-resource LabRun
`G:\SS\lab\runs\stage2-native-transform-clean-01` loaded the existing
`DB_OLD` mission slot to `LOAD-SLOT-DONE`, accepted `quit`, exited normally,
and left no crash dump. This is a Windows regression, not a Linux mission
load. The temporary x86 comparison build at
`G:\SS\lab\build-x86-transform` can be discarded after recording its values;
x86 is not a supported product build.

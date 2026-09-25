# Native terrain spline on Linux

Stage 2 now compiles the game's `Main/BetaSpline.cpp` for Windows x64 and
Linux x86-64/ARM64. `NWorld::CHeightLayers::ComputeLayers` uses
`CBetaSpline::Value(CArray2D<float>&, int, int)` while smoothing terrain-height
knots. The original Windows `Main` target still compiles this source; Linux
uses a separate `s2_game_beta_spline` target. Only platform include paths and
the headless include preamble changed; the spline formulas did not.

`NativeBetaSplineTests` evaluates a deterministic 7x7 non-planar height grid:
interior average and smoothed knot, boundary fallback, a 4x4 surface patch,
and its derivatives. It pins representative numerical values. Windows x86
(supplemental recovered-source oracle), Windows x64, Linux GCC x86-64, and
Linux Clang 14 x86-64 printed:

```
point=1.21322346,1.52093554,0.234747231 derivative=0.747696161,-0.0673760623 average=2.9366281 knot=2.01249027
```

ARM64/QEMU matched within one or two float ULPs: `du.z=0.747696042` and
`knot=2.01249003`; the other printed fields matched exactly. The test uses
an absolute tolerance of `1e-4`. The recovered x86 code verifies the
focused implementation; it does not substitute for an original Steam
gameplay comparison.

On the current Windows host:

```powershell
$env:UCRTContentRoot='C:\Program Files (x86)\Windows Kits\10\'
$cmake='G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
& $cmake --build 'G:\SS\lab\build-x64-stage2' --config RelWithDebInfo --target NativeBetaSplineTests --parallel 12
& 'G:\SS\lab\build-x64-stage2\RelWithDebInfo\NativeBetaSplineTests.exe'
```

On the authorized Linux host, use the configured portable-core build trees:

```sh
cmake --build /tmp/s2-matrix-links.zCkO0O/build-x64 --target NativeBetaSplineTests -j 16
ASAN_OPTIONS=detect_leaks=0 /tmp/s2-matrix-links.zCkO0O/build-x64/NativeBetaSplineTests
cmake --build /tmp/s2-matrix-links.zCkO0O/build-arm64 --target NativeBetaSplineTests -j 16
ASAN_OPTIONS=detect_leaks=0 qemu-aarch64-static -L /usr/aarch64-linux-gnu /tmp/s2-matrix-links.zCkO0O/build-arm64/NativeBetaSplineTests
```

The full suite passed after adding the test: Windows x64 87/87, Linux GCC
x86-64 57/57 under ASan/UBSan, GCC ARM64/QEMU 57/57 under ASan/UBSan, and
Clang 14 x86-64 release 57/57. The full height-layer cache,
path-network rasterization, mission load, and camera use of terrain heights
are not yet in a Linux game target; this is one original game-used kernel,
not a complete terrain or world port.

Clean native-media Windows x64 archive
`G:\SS\lab\builds\stage2-native-terrain-spline-20260925-01` came from
source commit `5de426d`. A fresh linked-resource LabRun
`G:\SS\lab\runs\stage2-native-terrain-spline-clean-01` loaded the existing
`DB_OLD` mission slot to `LOAD-SLOT-DONE`, accepted `quit`, exited normally,
and produced no crash dump. This verifies Windows game integration after
the shared-source include change; it does not prove Linux mission loading.

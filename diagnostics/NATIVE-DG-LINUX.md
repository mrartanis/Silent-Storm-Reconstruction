# Native dependency-graph frame runtime on Linux

Stage 2 now compiles the game's `Main/DG.CPP` as `s2_game_dg` on Linux x86-64
and ARM64. The Windows `Main` target still builds the same source. `DG` is
the frame/version and deferred-ownership substrate used by world and terrain
data functions; `CHeightLayers` itself is not yet in the Linux target.

The Linux source uses the headless FileIO preamble and original `DG.H`.
Three `UpdateSet` templates now name their dependent iterator types, as
required by GCC/Clang. `SetToHoldQueue` retains the original 32-bit delay
sequence but computes it with unsigned modular arithmetic before copying the
bits back to the existing `int` global. This avoids undefined signed overflow
while preserving the MSVC/x86 bit pattern and the 64–127-frame delay.

`NativeDGTests` exercises `CDGPtr` version refresh within and across frames,
deferred object hold, and six delay-generator updates (including a value
above `INT_MAX`). Linux GCC x86-64/ARM64 and Clang x86-64 printed:

```
frame=101 value=2 rng=2676965653 due=186
```

Windows x86 and x64 printed the same values. Windows x86 is a supplemental
recovered-source comparison, not a supported product target or a substitute
for the original Steam gameplay reference.

On the current Windows host:

```powershell
$env:UCRTContentRoot='C:\Program Files (x86)\Windows Kits\10\'
$cmake='G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
& $cmake --build 'G:\SS\lab\build-x64-stage2' --config RelWithDebInfo --target NativeDGTests --parallel 12
& 'G:\SS\lab\build-x64-stage2\RelWithDebInfo\NativeDGTests.exe'
```

On the authorized Linux host, use the configured portable-core build trees:

```sh
cmake --build /tmp/s2-matrix-links.zCkO0O/build-x64 --target NativeDGTests -j 16
ASAN_OPTIONS=detect_leaks=0 /tmp/s2-matrix-links.zCkO0O/build-x64/NativeDGTests
cmake --build /tmp/s2-matrix-links.zCkO0O/build-arm64 --target NativeDGTests -j 16
ASAN_OPTIONS=detect_leaks=0 qemu-aarch64-static -L /usr/aarch64-linux-gnu /tmp/s2-matrix-links.zCkO0O/build-arm64/NativeDGTests
```

This is not the Linux world graph, terrain cache, or game loop. The next
world-module boundary includes `TerrainInfo`, `aiGrid`/path-network tiles,
`CHeightLayers`, and their game-data ownership and save registration.

Full CTest after this change: Windows x64 88/88, Linux GCC x86-64 58/58
under ASan/UBSan, Linux GCC ARM64/QEMU 58/58 under ASan/UBSan, and Linux
Clang 14 x86-64 release 58/58.

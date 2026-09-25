# Original map polygon clipping (stage 2)

`Main/PolyUtils.cpp` now compiles as the original game implementation on
Linux. `NativeMapPolygonTests` calls its actual `IsPolygonInverse` and
`ClipPolygon` functions. Two overlapping 4 x 4 squares produce one
intersection of area 4 and one remainder of area 12. The same result was
observed on Windows x86/x64, Linux GCC x64/ARM64, and Linux Clang x64:

```
intersection=1 remainder=1 areas=4.000000,12.000000
```

The ARM64 ASan/UBSan run initially exposed an invalid read of an
uninitialized `EPolygon` when the original `CRing<SEdge>::add()` copied a
default edge. Both default `SEdge::polygon` and `SLink::eType` are now
initialized to `POLY_SOURCE`. The sanitized ARM64 test then passed. These
defaults do not change an inserted edge's intended type: `AddPolygon`
assigns it before clipping.

On Windows the project header `Main/Time.h` collided, case-insensitively,
with the C runtime `<time.h>` in fresh builds. It is now `Main/A5Time.h`,
with direct includes updated. `Main/StdAfx.h` includes it explicitly because
some original translation units had accidentally received `STime` through
the C-header collision. A full Windows x64 RelWithDebInfo build including
`Game.exe` then succeeded, and CTest passed 98/98. The x86 polygon target
rebuilt and produced the same result. Linux GCC x64 and ARM64 and Clang x64
each passed their full 72/72 test suites after the rename.

Reproduce with `cmake --build <build> --target NativeMapPolygonTests` and
`ctest --test-dir <build> -R '^NativeMapPolygonTests$' --output-on-failure`.
Use `--config RelWithDebInfo` for the Windows multi-config build. The ARM64
build here runs through `qemu-aarch64-static -L /usr/aarch64-linux-gnu` with
`ASAN_OPTIONS=detect_leaks=0` because LeakSanitizer cannot inspect the
emulated process.

This is a map-building dependency, **not** a successful full mission build.
An experimental link of original `BuildMap` after adding `PolyUtils.cpp`
still lacked game-used building-grid, building-info, solid/wall-map and
terrain functions (`CBuildingGrid::Setup`/`Explode`, `MakeSWMap`,
`BuildingHP`, `CBuildInfoLoader`, `BlendTerrainInfo`, `LoadRootTerrain`,
`MakeSoundMap`, `CalcAverageColor`, `GetMeterHeightCheck`, and
`ClearTerrainCache`). The first three groups are implemented in the
original `BuildingGrid.cpp`, `BuildingInfo.cpp`, `MakeBuilding.cpp`, and
`GBuilding.cpp`; the terrain group is in `MapBuildTerrain.cpp` and its
dependencies. They have not yet been linked and executed on Linux. No
`BuildTerrain`/MapEdit-only surrogate counts as completing this game path.

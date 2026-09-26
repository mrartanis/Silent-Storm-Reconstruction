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
An experimental link of original `BuildMap` immediately after adding
`PolyUtils.cpp` lacked building-grid, building-info, solid/wall-map, and
terrain functions. The later building/terrain batch now compiles and
directly tests `BuildingGrid.cpp`, `BuildingInfo.cpp`, and
`MapBuildTerrain.cpp` on Linux (see `NATIVE-BUILDING-TERRAIN-LINUX.md`).
`MakeSWMap`/`BuildingHP` in `MakeBuilding.cpp` and their solid/wall-map
dependencies remain for a full original `BuildMap` mission link. No
`BuildTerrain`/MapEdit-only surrogate counts as completing this game path.

Clean Windows x64 native-media archive
`G:\SS\lab\builds\stage2-map-polygons-20260925-01` was built from
`c4f2fe9`; its `Game.exe` SHA-256 is
`CD9DBD576297BBCAF87633415BA7FB8130DDAF91D8E582783369A92E24B73556`.
The fresh linked-resource runs `stage2-map-polygons-clean-01` and `-02`
did **not** reach `LOAD-SLOT-DONE`: the current Windows graphics session
returned `0x88760868` from D3D device creation, after which the old
`ResetDevice` code dereferenced a null device at `Gfx.cpp:138`. An unchanged
earlier archive (`stage2-resource-loader-20260925-01`), which had previously
loaded `DB_OLD` successfully, failed at the identical location in a fresh
`stage2-resource-loader-current-session-01` run. This is a current-session
graphics gate, not a passing smoke and not evidence of a new polygon
regression. Repeat the archived game smoke when a working D3D desktop is
available. The dumps are retained in the corresponding `evidence` folders.

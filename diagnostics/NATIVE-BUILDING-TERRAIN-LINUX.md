# Original building grid and terrain resources (stage 2)

The Linux portable-core build now compiles the game's original
`BuildingGrid.cpp`, `BuildingSchema.cpp`, `RodJunction.cpp`, `rod.cpp`,
`BuildingInfo.cpp`, and `MapBuildTerrain.cpp`. These are linked as original
game code, not substitutes for a mission build. The renderer-side
`GBuilding.cpp` is still Windows-only; Linux provides its resource share
registration in `BuildingInfo.cpp` with the same share ID (108).

`NativeBuildingGridTests` exercises HP damage, destroyed voxels, updated
building parts, cellar and indestructible spots, and an explosion. It found
an original shadowed local in `CBuildingGrid::UpdatePart`: `SPoint3 pt(
pt.x + x, ...)` read an uninitialized local on GCC. Giving the neighbor
a distinct name made the result deterministic across platforms. Windows
x86/x64, Linux GCC x64/ARM64, and Linux Clang x64 report eight updated
parts, twelve explosion-destroyed cells, and the same HP hash
`776DA3697E4E585D`.

`NativeBuildingTerrainResourceTests` loads the real `game.db`, intersects
the game's template variants with `Buildings.res` and `Terrain.res`, then
uses `CBuildInfoLoader`, `CMETerrainLoader`, `LoadRootTerrain`,
`MakeSoundMap`, and `CalcAverageColor`. There are 2012 resource-matched
variants. The first 29 sorted candidates contain 21 empty/non-geometric
variants; eight nontrivial variants have a shared semantic digest
`E28FBA0B16F6B77F` on Windows x86/x64 and Linux GCC x64/ARM64 and Clang
x64. GCC's original building-info loader also reports three
`CBuildInfoLoader::Recalc()` exceptions among those empty candidates; the
test counts them as empty only after validating the returned objects. This
probe is deliberately bounded to eight nontrivial variants, not an
exhaustive audit of all 2012.

Reproduce the direct tests with `cmake --build <build> --target
NativeBuildingGridTests NativeBuildingTerrainResourceTests`, then
`ctest --test-dir <build> -R '^NativeBuilding(Grid|TerrainResource)Tests$'
--output-on-failure`. On Windows add `--config RelWithDebInfo` and pass
`-C RelWithDebInfo` to CTest. Resource tests need `game.db` and both `.res`
packages in the configured resource directory. The GCC ARM64 run uses
`qemu-aarch64-static` and `ASAN_OPTIONS=detect_leaks=0`; GCC x64/ARM64
builds use ASan/UBSan.

This still does not mean `BuildMap` has linked and executed an actual
mission on Linux. Its next unresolved game-side dependencies include the
solid/wall map and building composition in `MakeBuilding.cpp` and
`GBuilding.cpp`. The Windows game smoke for this batch is postponed until
a D3D device can be created in the current desktop session; an unchanged
earlier archive also failed at `NGfx::ResetDevice` before loading a save.

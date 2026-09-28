# Original building grid and terrain resources (stage 2)

The Linux portable-core build now compiles the game's original
`BuildingGrid.cpp`, `BuildingSchema.cpp`, `RodJunction.cpp`, `rod.cpp`,
`BuildingInfo.cpp`, and `MapBuildTerrain.cpp`. The next batch also links
original `MakeBuildingInternal.cpp`, `MakeBuilding.cpp`,
`BuildingClip.cpp`, and `aiObjectLoader.cpp`. These are original game code,
not substitutes for a mission build. The renderer-side
`GBuilding.cpp` is still Windows-only; Linux provides its resource share
registration in `BuildingInfo.cpp` with the same share ID (108). Likewise,
`aiObjectLoader.cpp` owns the Linux AI-geometry share with original ID 110
while renderer-heavy `aiMap.cpp` remains out of the headless core.

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
`MakeSoundMap`, and `CalcAverageColor`. It also requires
`AIGeometries.res`, constructs the game's `CSolidAndWallMap` via
`MakeSWMap`, runs `BuildingHP`, and counts nonzero HP voxels. There are 2012 resource-matched
variants. The first 29 sorted candidates contain 21 empty/non-geometric
variants; eight nontrivial variants have a shared semantic digest
`0EC6288794E257DD` and 756 nonzero HP voxels on Windows x86/x64,
Linux GCC x64/ARM64, and Clang x64. GCC's original building-info loader also reports three
`CBuildInfoLoader::Recalc()` exceptions among those empty candidates; the
test counts them as empty only after validating the returned objects. This
probe is deliberately bounded to eight nontrivial variants, not an
exhaustive audit of all 2012.

Reproduce the direct tests with `cmake --build <build> --target
NativeBuildingGridTests NativeBuildingTerrainResourceTests`, then
`ctest --test-dir <build> -R '^NativeBuilding(Grid|TerrainResource)Tests$'
--output-on-failure`. On Windows add `--config RelWithDebInfo` and pass
`-C RelWithDebInfo` to CTest. Resource tests need `game.db` and both `.res`
packages plus `AIGeometries.res` in the configured resource directory. The GCC ARM64 run uses
`qemu-aarch64-static` and `ASAN_OPTIONS=detect_leaks=0`; GCC x64/ARM64
builds use ASan/UBSan.

The original `BuildMap` now links and runs on a one-building variant;
see `NATIVE-MISSION-BUILD-LINUX.md` for its narrow coverage and the next
mission gate. The Windows game smoke for this batch is postponed until
a D3D device can be created in the current desktop session; an unchanged
earlier archive also failed at `NGfx::ResetDevice` before loading a save.

## Scenario-reachable typed-resource follow-up (2026-09-28)

The earlier eight nontrivial IDs (`218,491,505,687,751,770,792,810`)
do **not** occur in the 985-variant closure of the 52 shipped scenario
roots. `NativeBuildingTerrainResourceTests` now additionally walks those
scenario DB roots, uses the game's effective resource lookup (loose before
package), and strictly deserializes every **present** `CBuildInfo` and
`CMETerrainInfo` in that closure. It checks dimensions/floor order and
hashes IDs, dimensions, fragment/spot/ladder counts, cellar sizes and
terrain-array sizes. Windows x64 `RelWithDebInfo` prints and asserts:

```text
scenario_typed variants=985 buildings=903 terrain=371 failed=0 building_digest=A924C3A98CD24D04 terrain_digest=A29501ED83DC7587
```

The five loose building overrides are included. The baseline copy's
`game.db`, `Buildings.res`, `Terrain.res`, and those five files match the
installed Steam originals by SHA-256. The ordinary Windows x64 CTest
`NativeBuildingTerrainResourceTests` passed after this addition. This is
strict type decoding plus a summary-field regression for the potential
scenario set, **not** full hashing of every nested material/voxel value,
not a test that each random variant is selected, and not evidence that
absent variant IDs should contain a resource. The new branch has not yet
been run on Linux x86-64, ARM64/QEMU, or diagnostic x86; those comparisons
remain required before cross-platform closure.

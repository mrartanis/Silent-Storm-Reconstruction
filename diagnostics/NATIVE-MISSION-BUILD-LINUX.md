# Original mission `BuildMap` on Linux (stage 2)

`NativeMissionMapProbe` links and executes the game's original
`Main/MapBuild.cpp` and its terrain, building-grid, solid/wall-map,
building-HP, AI-geometry, waypoint, and path-network dependencies. This is
the game `BuildMap` path, not editor-only `BuildTerrain` or a replacement
builder. The Linux target is built by default; the data test is registered
when `game.db`, `Waypoints.res`, `Buildings.res`, `Terrain.res`, and
`AIGeometries.res` are configured. Two further cases use `Units.res` and
`Groups.res`. Windows builds the same probe against
`Main`.

For template variant 218 from the baseline `game.db`, all tested builds
return `built=1`, one building, 21 items, no units, and no waypoints. A
semantic digest of the terrain corner, building variant and position,
script/group counts, unit/waypoint and clue-slot data, and item
model-parent IDs, positions and flags is `751202F4B6E394E0` on
Windows x86/x64 and Linux GCC x64/ARM64 and Clang x64. This precise case is a
single-building map without units or routes.

Variant 810 adds two units, two waypoints, one building and 24 items.
The probe includes script/group counts, unit IDs/positions/route lengths,
and waypoint positions/command lengths in its digest. Windows x86/x64
and Linux GCC x64/ARM64 and Clang x64 report `558E222D9ED26DA6`.
This proves construction of a small
unit/route map, not script execution or live enemy AI.

Variant 4526 is a larger authored map: seven buildings, 49 units, 24
waypoints, 1514 items, two clue slots, and one attached script. A fixed
builder seed makes the generated item count and digest repeatable.
Windows x86/x64 and Linux GCC x64/ARM64 and Clang x64 agree on
`8C92AD1F7E8B90FA`.
The first Linux run crashed in `GetMeterHeightCheck`: a unit-derived
`SClueSlot` had never received `ptAlignTo`, so a NaN coordinate reached
bilinear terrain sampling. `AddSimpleElements` now fills that coordinate
using the same alignment rule as other placed objects. Two subsequent
GCC x64 ASan/UBSan runs completed with matching digests without a
height-sampling guard or diagnostic suppression.

After the three cases were registered, the full Windows x64 build and
CTest passed 103/103. Linux GCC x64/ARM64 and Clang x64 passed 77/77;
GCC x64 and ARM64 used ASan/UBSan. Windows x86 passed the building/resource
test and all three mission cases. The ARM64 large-map test took about
97 seconds under QEMU, so its CTest timeout is 300 seconds.

The first Linux ASan/UBSan run reported an invalid `bool` value while
copying `SMapElement`. Its constructors left `bLightmap` and several other
primitive fields uninitialized for building objects and border items.
Both constructors now initialize those fields without changing layout.
The repeated GCC x64 run completed without a sanitizer diagnostic and
matched the Windows x86/x64 digest. GCC x64/ARM64 builds use ASan/UBSan;
the ARM64 suite runs under QEMU with `ASAN_OPTIONS=detect_leaks=0`.

Reproduce on Linux with `cmake --build <build> --target
NativeMissionMapProbe` and `ctest --test-dir <build> -R
'^NativeMission(MapProbe|RouteMapProbe|LargeMapProbe)$' --output-on-failure`. Configure
`S2_GAME_DB_PATH` and `S2_RESOURCE_PACKAGE_PATH` so the seven original
files named above are available. On Windows use `cmake --build <build>
--config RelWithDebInfo --target NativeMissionMapProbe` and `ctest
--test-dir <build> -C RelWithDebInfo -R '^NativeMission(MapProbe|RouteMapProbe|LargeMapProbe)$'
--output-on-failure`; configure `S2_GAME_DIR` with its `game.db` and `res`
folder. The probe also accepts explicit arguments `<game.db> <res-dir>
<variant-id>`. IDs 218, 810, and 4526 have asserted regression digests;
ID 0 lists
database candidates with units and waypoints.

The next stage-2 parity gate is script execution and runtime AI interaction
in a loaded mission, not just map construction. The
separate Windows `Game.exe` smoke remains deferred until the current D3D
desktop can create a device; an unchanged older archive failed at the
same `NGfx::ResetDevice` location before save loading.

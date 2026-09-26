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

The game `BuildMap` route-resolution path is now checked separately from
raw `Units.res`/`Groups.res` decoding. Variant 2400 creates five units and
assigns all five a route (ten `CMapWaypoint` links total). A route digest
over each unit ID, ordered waypoint-name ID, authored/pseudo-waypoint flag,
and command count is `BDADCF3CD005A847`; the map digest is
`29C99EC264C4C6E7`. Variant 4526 has no unit route in its selected
placement but resolves a fourteen-point group route; route digest
`C9FCDC98D9DCD6F3`. The small variant 810 happens to have no resolved
route despite containing two units, so its test cannot serve as the unit
route case. Windows x86/x64 and Linux GCC x64/ARM64 match these route digests.
This proves the original builder consumes the original route resources and
resolves names through `game.db`; actual per-turn AI route execution remains
unverified.

The route digest above checks only the number of waypoint commands. An
additional `behavior_digest` now covers every resolved route point's
position, floor, name, existence flag, and full command payload (kind,
position, time, pose, direction); all placed units' person template,
initial pose/logic, roaming and fear settings, guard animation, diplomacy
and scenario player; ordered group membership; all map waypoints; and the
exact attached Lua script bytes. The Windows x86 reconstructed build gives
`CBF29CE484222325` (218), `1150A6A5921D1F4D` (810),
`12659D562097CF78` (2400), and `7EAF4F97AD129EED` (4526).
Windows x64 and Linux GCC x64/ARM64 assert these values on the same
baseline data. This is stronger static map-data parity, not evidence that
the original Steam EXE or a live AI turn makes the same decisions.
The four cases passed on Windows x86/x64 and Linux GCC x64/ARM64;
the full Windows x64 CTest passed 107/107 and Linux x64/ARM64 passed
80/80 each with ASan/UBSan. The ARM64 suite ran under QEMU with
`ASAN_OPTIONS=detect_leaks=0`.

The expanded four-case mission set passed on Windows x86/x64 and Linux GCC
x64/ARM64. In the same packet the full Windows x64 build/CTest passed
107/107 and Linux GCC x64/ARM64 passed 80/80 under ASan/UBSan.

The probe now also passes every script attached by `BuildMap` through the
game's original Lua parser, using `CScript::strCode` from `game.db`. Variant
810 has one 1,016-byte script; variant 4526 has one 4,266-byte script. The
probe asserts these sizes in addition to successful parsing. This bridges
the authored mission data and the native Lua VM; it does not register the
game-specific bindings or execute mission commands. Those remain a stage-2
runtime gate. The linked parser cases passed on Windows x64 and Linux GCC
x64/ARM64; full CTest passed 103/103 on Windows x64 and 77/77 on Linux GCC
x64. All three ARM64 cases passed under QEMU. Clang was not reverified for
this incremental Lua link because the current Linux host's Clang toolchain
could not locate standard C++ headers; the previous map-only Clang results
below remain historical.

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

After the initial three cases were registered, the full Windows x64 build and
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
'^NativeMission(MapProbe|RouteMapProbe|UnitRouteMapProbe|LargeMapProbe)$' --output-on-failure`. Configure
`S2_GAME_DB_PATH` and `S2_RESOURCE_PACKAGE_PATH` so the seven original
files named above are available. On Windows use `cmake --build <build>
--config RelWithDebInfo --target NativeMissionMapProbe` and `ctest
--test-dir <build> -C RelWithDebInfo -R '^NativeMission(MapProbe|RouteMapProbe|UnitRouteMapProbe|LargeMapProbe)$'
--output-on-failure`; configure `S2_GAME_DIR` with its `game.db` and `res`
folder. The probe also accepts explicit arguments `<game.db> <res-dir>
<variant-id>`. IDs 218, 810, 2400, and 4526 have asserted regression digests;
ID 0 lists
database candidates with units and waypoints; ID -1 lists placements whose
unit IDs have entries in `Units.res` (a candidate can still have an empty
route). ID 2400 is the unit-route regression.
Pass `--print-scripts` after a variant ID to inspect its attached authored
Lua source locally when deciding which game bindings to port; the source is
not included in this repository.

The next stage-2 parity gate is script execution and runtime AI interaction
in a loaded mission, not just map construction. The
separate Windows `Game.exe` smoke remains deferred until the current D3D
desktop can create a device; an unchanged older archive failed at the
same `NGfx::ResetDevice` location before save loading.

A clean native-media Windows x64 archive
`G:\SS\lab\builds\stage2-mission-map-20260926-01` was built from source
commit `111dc78b0b35700023483b14c401b5a4c764ccca` with 16 build jobs.
Its `Game.exe` SHA-256 is
`447E33CA0D8AC27FB63E26CD767D5C2BA0465987E5E1A2A031A5217B131A2641`.
The archive contains no FMOD DLL and `Game.exe` has no FMOD import. The
archive build is verified; the D3D-gated game runtime smoke is not.

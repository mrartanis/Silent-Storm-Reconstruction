# Scene data used by the stage-2 core

`NGScene::MergePositions` and `FilterTrinagles` were moved without
algorithm changes from `GGeometry.cpp` to `GGeometryCore.cpp`. The new
translation unit is shared by the Windows game and Linux
`s2_game_scene_data` target. The latter also compiles the original
`aiTerrain.cpp`, `GBind.cpp`, and `GMesh.cpp`: terrain collision geometry,
resource-backed skeleton bindings, and mesh bounds used by game objects.
`wDecal.cpp`, the game's hit/decal state object, now compiles in
`s2_game_world_gameplay` on Linux. None of this replaces the renderer or
implements SDL3/bgfx integration.

`NativeGeometryCoreTests` checks duplicate-position remapping and
degenerate-triangle removal. Its output is the same on restored Windows
x86, Windows x64, Linux x86-64, and Linux ARM64/QEMU:
`positions=3 mapping=0,1,2,0 triangles=2`. This is parity of this
original algorithm on a focused
fixture, not Steam gameplay parity. The other newly compiled scene
classes do not yet have a live resource/mission regression test.

Windows x64 `RelWithDebInfo` builds `Game.exe` and passes 117/117 CTest
cases. Linux GCC x86-64 and ARM64 build all targets and pass 91/91
tests each under ASan/UBSan (ARM64 under QEMU).

The diagnostic link of the real `NativeAILogicTests` object against all
current Linux game archives still fails on eight distinct unresolved
symbols, down from 15 before this package. These include the head
resource constructor, Lua UI registration, the complete game Lua
function table, debug particle storage, and scene/decal serialization
types. Linking the full Linux game or running a live mission remains
open. No fake symbols or unresolved-symbol linker bypass were used.

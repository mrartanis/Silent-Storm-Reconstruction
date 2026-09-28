# Stage-2 shipped-resource roots (item 2a, in progress)

Scope: the base Steam game. Shipped mods are optional and arbitrary
external mods are not a gate. This ledger identifies **source-code entry edges** into the
23 base `.res` families. It does not yet enumerate every ID selected from
`game.db`/Lua, prove that every family is reached in normal play, or equate
byte-level package coverage with typed coverage. Stage-2 closeout item 2
requires those later joins.

The input baseline is `G:\SteamLibrary\steamapps\common\Silent Storm\res`.
`PORTABLE-RESOURCE-CORPUS.md` records all 23 packages (57,880 indexed
entries); `NATIVE-RESOURCE-LOADER-LINUX.md` documents the actual lookup
order: newest loose file, newest package, base package. This order is part of
the graph and means a `.res`-only scan can select the wrong payload.

| Family | Concrete game/source entry edge | Existing typed evidence / stage-2 boundary |
|---|---|---|
| `AIBSPTrees` | `aiMap.cpp` door hull → `shareBSPTrees` → `CLoadTwoBSPTrees` | `NativeAIBSPResourceTests`; current precalc collision, **not** old `BSPTree.cpp` |
| `AIBinds` | `aiMap.cpp` AI geometry bind → `shareAIBinds` → `CFileAIBind` | `NativeAIBindResourceTests` and pose tests |
| `AIGeometries` | `aiMap.cpp`, `MapBuild.cpp`, `wBuilding.cpp` → `shareAIModel` → `CLoadGeometryInfo` | `NativeAIGeometryResourceTests`; active collision/building data |
| `Animations` | `GAnimation.cpp` DB animation record ID → `shareAnimations` → `CFileAnimation` | `NativeAnimationResourceTests` checks effective loose/package data |
| `Binds` | `GView.cpp` model geometry ID → `shareBinds` → `CFileBind` | `NativeAIBindResourceTests`; model skinning data; display itself later |
| `Buildings` | `MapBuild.cpp`/`MapBuildTerrain.cpp` variant ID → `shareBuildings` → `CBuildInfoLoader` | `NativeBuildingPieceResourceTests`, `NativeBuildingTerrainResourceTests`; game build/destruction data |
| `Chapters` | `iChapterMap.cpp` global chapter ID → `shareChapterInfo` → `CChapterInfoLoader` | `NativeCampaignMapResourceTests` proves the normal side 1/2 reachable subset |
| `Effects` | `GView.cpp` DB particle effect ID → `shareParticles` → `CParticlesLoader` | `NativeParticleRuntimeResourceTests` covers CPU keys; GPU particles deferred |
| `Fonts` | `GLocale.cpp` DB font ID → `shareFonts` → `CFileFont` | client text display; no stage-2 world decoder requirement established |
| `Geometries` | `GView.cpp` DB model/part key → `shareObjInfo`; `GObjectInfo.cpp` → `CObjectInfoLoader` | `NativeModelGeometryResourceTests` decodes all 6,824 DB-linked package parts; 966 other entries remain unclassified, rendering later |
| `Globals` | `iGlobalMap.cpp` campaign map ID → `shareGlobalInfo` → `CGlobalInfoLoader` | `NativeCampaignMapResourceTests`; normal side 1/2 IDs 3/4 strict |
| `Groups` | `MapBuild.cpp` group ID → `shareUnitGroups` → `CUnitGroupAIInfoLoader` | `NativeAIRouteResourceTests`; route waypoint-name references |
| `Heads` | `LSHeadPortable.cpp` DB head ID → `shareHeads` → `HeadResourceData.cpp` | `NativeHeadResourceTests`; game-used CPU face data, display later |
| `LRTextures` | `GTexture.cpp` texture ID → `CResourceFileOpener("LRTextures", ...)` | low-resolution visual fallback; graphics stage |
| `Lights` | `GView.cpp` DB light ID → `shareAnimLights` → `CLightLoader` | animated-light WIP preserved, further work explicitly deferred to graphics stage |
| `Locators` | `GAnimation.cpp` geometry ID → `shareLocators` → `CFileLocators` | `NativeAnimationResourceTests` |
| `Sequences` | `LSHeadPortable.cpp` DB facial sequence ID → `shareSequences` | `NativeHeadSequenceRuntimeTests`; CPU sequence playback |
| `Skeletons` | `GAnimation.cpp` DB skeleton ID → `shareSkeletons` → `CFileSkeleton` | `NativeAnimationResourceTests` |
| `Sounds` | `Sound.cpp`/`SoundEffect.cpp` DB sample ID → `share2DSamples`/`share3DSamples` → `SoundFormat.cpp` | Windows native audio/media path in `PORTABILITY-MEDIA.md`; headless Linux audio is not a world rule |
| `Terrain` | `MapBuildTerrain.cpp` variant ID → `shareTerrains` → `CMETerrainLoader` | `NativeBuildingTerrainResourceTests`; CPU terrain/build data |
| `Textures` | `GTexture.cpp` DB texture ID → `CFileRequest("Textures", ...)` | visual texture decoding/upload deferred to graphics stage |
| `Units` | `MapBuild.cpp` unit ID → `shareUnits` → `CUnitAIInfoLoader` | `NativeAIRouteResourceTests` |
| `Waypoints` | `MapBuild.cpp` DB waypoint ID → `shareWaypoints` → `CWaypointLoader` | `NativeWaypointResourceTests`; two package records are known strict-decode exceptions |

The table is an entry-edge inventory, not the requested final reachability
graph. Next bounded step: enumerate the source `game.db` record IDs and Lua
constants feeding the *non-visual* loader edges above, then resolve each
against effective loose/package payloads. The existing corpus tests should
be reused as evidence only for the actual ID/type set they cover. Keep the
known campaign-map and waypoint exceptions explicit; do not silently treat
all 57,880 indexed records as game-used typed objects.

Reproduce the static edge inventory with `rg` over `Main/` for
`CResourceOpener`, `CFileRequest`, `share*.Get`, and the 23 package names.
The authoritative source list for this stage is the committed
`sources.cmake` `Main_SRC`, not all historical files in `Main/`.

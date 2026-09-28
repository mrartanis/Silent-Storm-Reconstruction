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

## First reproducible DB-ID join: scenario map closure

`NativeMapDatabaseTests <game.db> --resource-roots` walks the 53 scenario
zones → 52 authored root templates → child-template graph → 985 potential
variants. For each variant it gathers the IDs consumed by the actual
`MapBuild` loader edges: variant ID for `Buildings`/`Terrain`, waypoint
record ID, unit record ID, unit-group record ID, and guard-animation ID.
It also records variant script IDs as the first Lua roots. The option pins
counts and FNV-1a-64 hashes of sorted 32-bit IDs, so drift fails the
diagnostic. `--resource-root-ids` prints every edge as
`resource_root family=<name> id=<id>` for a later effective-file join.

On the installed Steam `game.db`, Windows x64 `RelWithDebInfo` gives:

```text
resource_roots family=Buildings count=985 digest=C568A7F7EB837585
resource_roots family=Terrain count=985 digest=C568A7F7EB837585
resource_roots family=Waypoints count=1357 digest=4347512C8B8B7C62
resource_roots family=Units count=1117 digest=F03726B7CCC79175
resource_roots family=Groups count=97 digest=B3B458E2B8360B13
resource_roots family=Animations-guard count=4 digest=830893280521BE14
resource_roots family=Lua-variant count=47 digest=20F53D173CF351DE
```

The 1,357 waypoint candidate IDs do **not** include package IDs 8 or 9,
the two strict-decode exceptions in `NativeWaypointResourceTests`. That
narrows the normal scenario path; it does not excuse those records for
custom content. `Units.res` has only 169 package entries, so 1,117 DB unit
IDs cannot be misreported as 1,117 present route resources. A missing
route resource is an optional route, and the next join must distinguish
missing from decoded. Likewise, random template selection means this is a
*potential* scenario closure, not proof every listed ID is fetched in one
playthrough.

Reproduce on Windows after building `NativeMapDatabaseTests`:

```text
NativeMapDatabaseTests.exe "G:\SteamLibrary\steamapps\common\Silent Storm\game.db" --resource-roots
NativeMapDatabaseTests.exe "G:\SteamLibrary\steamapps\common\Silent Storm\game.db" --resource-root-ids
```

The normal `NativeMapDatabaseTests` CTest now **also asserts** these seven
counts and digests without printing them, and passes on Windows x64.
The printable option has so far been executed on Windows x64 only; Linux x86-64 and ARM64
verification remains for the stage-2 matrix. Next step within item 2:
resolve these exact IDs through the game's loose/package precedence and
compare effective payloads to the corresponding typed tests.

## Base-game effective-file join (item 2b, first six families)

`Compare-Stage2ResourceRoots.ps1` joins the exact IDs above with the
`PortablePackageProbe --list` indexes and numeric files in each matching
loose directory. Its precedence is loose file before package, matching
`CResourceFileOpener`. `-List` emits one `resource_effective` line per
candidate ID; `-StrictBaseline` checks both the candidate resolution counts
and the full package-index entry counts. It does not read/decode payloads,
select a random variant, or apply optional shipped-mod overlays.

Windows x64 against the installed Steam base data:

```text
resource_resolution family=Buildings candidates=985 package=898 loose_override=5 loose_only=0 missing=82
resource_resolution family=Terrain candidates=985 package=371 loose_override=0 loose_only=0 missing=614
resource_resolution family=Waypoints candidates=1357 package=1285 loose_override=0 loose_only=0 missing=72
resource_resolution family=Units candidates=1117 package=62 loose_override=8 loose_only=0 missing=1047
resource_resolution family=Groups candidates=97 package=34 loose_override=0 loose_only=0 missing=63
resource_resolution family=Animations-guard candidates=4 package=4 loose_override=0 loose_only=0 missing=0
```

There are 4,545 candidate ID edges in these six families. The five
effective loose building records are IDs 3761, 5123, 5235, 5264 and 5300.
The 72 absent waypoint IDs are 1768–1837 and 2991–2992; these are
different from the malformed *present* records 8 and 9. `CWaypointLoader`
returns without a value on absent input and `CMapBuilder::AddWaypoints`
skips it. `CUnitAIInfoLoader`/`CUnitGroupAIInfoLoader` similarly return
without routes for absent records; `CMETerrainLoader` returns without a
value. The candidate closure is wider than a single actual map selection,
so these missing counts are data facts, **not yet claims that a normal
playthrough loses needed waypoints, routes, buildings or terrain**. That
requires tracing selected variants and loader results in the 52-root runs.

Reproduce without writing into the installed game:

```powershell
& .\diagnostics\Compare-Stage2ResourceRoots.ps1 `
  -GameDb 'G:\SteamLibrary\steamapps\common\Silent Storm\game.db' `
  -ResourceDir 'G:\SteamLibrary\steamapps\common\Silent Storm\res' `
  -BuildDir 'G:\SS\lab\build-x64-stage2\RelWithDebInfo' `
  -StrictBaseline
```

Add `-List` for all ID→effective-source lines. The next item-2 work is to
compare the *present* ID sets against the typed-test sets, investigate any
present-but-untyped records, and connect other game-used DB/Lua families.

Reproduce the static edge inventory with `rg` over `Main/` for
`CResourceOpener`, `CFileRequest`, `share*.Get`, and the 23 package names.
The authoritative source list for this stage is the committed
`sources.cmake` `Main_SRC`, not all historical files in `Main/`.

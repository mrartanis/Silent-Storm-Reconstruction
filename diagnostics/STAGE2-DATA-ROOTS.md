# Stage-2 shipped-resource roots (item 2 evidence ledger)

Scope: the base Steam game. Shipped mods are optional and arbitrary
external mods are not a gate. This ledger identifies **source-code entry edges** into the
23 base `.res` families. This is a bounded graph of source loader edges and
effective base-game data, not a claim that every ID is selected in a
playthrough or that byte-level package coverage implies typed coverage.
The 2026-09-28 final review below states which stage-2 data paths are
covered and which visual/client paths are deferred.

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
| `Geometries` | `GView.cpp` DB model/part key → `shareObjInfo`; `GBuilding.cpp` DB wall/solid key → `shareWallClippers`/`shareSolidClippers` → `CObjectInfoPiecesLoader` | `NativeModelGeometryResourceTests` decodes 6,824 DB-linked model parts; `NativeBuildingPieceResourceTests` decodes 2,928 construction-piece records and proves all 814 construction geometry IDs have DB rows. The 966 other package IDs have no DB geometry row and no identified base-game key path; rendering later. |
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

## Typed coverage of these six families (item 2c, in progress)

`NativeWaypointResourceTests` strictly checks every present package
waypoint except the recorded malformed 8/9, neither of which is in this
scenario candidate set. `NativeAIRouteResourceTests` checks all 169
`Units.res` and 42 `Groups.res` records; the 62+8 unit and 34 group
candidate records are subsets. `NativeAnimationResourceTests` covers
the effective animation corpus, including all four guard-animation IDs.
These claims concern present typed payloads, not whether every candidate
missing route/waypoint is desirable gameplay data.

The old `NativeBuildingTerrainResourceTests` selected eight nontrivial
building/terrain IDs, **none** in the 985-variant scenario closure. That
was a real coverage gap. The test now walks the scenario template graph
and strictly decodes all 903 present effective `Buildings` records and
371 present `Terrain` records, including the five loose building overrides.
It asserts summary-field hashes
`A924C3A98CD24D04`/`A29501ED83DC7587` and selected nested gameplay
hashes `0CF10374324B7718`/`5928DA62CE81B4F8` on Windows x64,
Linux GCC x86-64 and ARM64/QEMU; see `NATIVE-BUILDING-TERRAIN-LINUX.md`
for the exact tested fields and limits. Diagnostic x86 has not been
repeated for the new fields.
The larger graph still lacks ID joins for several other game-used resource
families, and nested field equivalence has not been claimed here.

## Next bounded join: collision-resource families

The next nonvisual family group is `AIGeometries`, `AIBinds`, and
`AIBSPTrees`, because `CAIMap` uses them for static, animated, and
flipping/door collision hulls. Existing typed evidence is unusually broad:
`NativeAIGeometryResourceTests` covers all 1,982 effective records named by
the game's AI-geometry DB table (1,974 loose overrides plus eight package
records); `NativeAIBindResourceTests` covers all 211 packaged inverse-pose
records; `NativeAIBSPResourceTests` covers all 193 effective loose door
collision records. Each existing semantic digest agrees on Windows x64,
Linux x86-64, ARM64/QEMU and diagnostic x86; details and input hashes are in
`NATIVE-AI-GEOMETRY-RESOURCES.md`. Thus this group's *present typed payload*
decode has no newly identified gap. The bounded DB join is now implemented
as the `--collision-roots` mode of `NativeAIGeometryResourceTests`: 61
package-only AI geometries have no `CAIGeometry` record and no identified
game source edge; six of 211 AI-bind IDs similarly lack an AI-geometry DB
record. All 193 BSP IDs correspond to `CDoor` IDs, but door 234 has no BSP
file and **does** occur in four potential scenario variants. The original
loader's empty-data fallback is pinned by `NativeAIBSPResourceTests`; the
door's dynamic collision outcome is not yet claimed. Exact IDs, hashes and
commands are in `NATIVE-AI-GEOMETRY-RESOURCES.md`. Do not infer reachability
from package membership. Dynamic door-state parity belongs to the later
behavior gate.

## Shipped Lua roots and actual source payload

`NativeMissionScriptCorpusTests` now joins the 47 distinct script IDs from
the 985 scenario-reachable variants (digest `20F53D173CF351DE`) to the
original `NDb::CScript` table. Across every variant, not only scenarios,
91 IDs are referenced; global maps reference two, chapter maps four, and
the hardcoded main menu launches ID 85. Persona and UI-container DB rows
have no links. Their union is 97 distinct IDs (digest
`7697CE69E943ED7E`), and **none** points to absent or empty source. The
game's `RunScriptByID` loads these texts from `game.db`; no separate file
selection is involved. All 113 DB script records (one empty) already pass
the modified Lua parser, with current source digest `C2462A66D562BAF6`.

The other shipped source edge is `CWorld::RunAutoLoadScripts`: four DB rows
name loose `.l` files. `NativeWorldInitProbe` resolves and executes all four
on Windows x64, Linux GCC x86-64 and ARM64/QEMU. The new DB-reference join
and existing world-init test match on those three targets under ASan/UBSan
on Linux. This closes the *source/selection* substep for these Lua roots,
not every callback or branch's runtime behavior. See `NATIVE-LUA-LINUX.md`
and `NATIVE-WORLD-INIT.md` for values and commands. The debug-only
`script_run` console command and arbitrary external mods are outside this
base-game data gate.

## Bounded 23-family disposition for stage 2

The preceding joins and existing typed probes give the following working
disposition. "Covered" means the stated *data/CPU loading* contract, not
every live mission outcome; the named diagnostics above retain exact scope.

| Disposition | Families | Remaining stage-2 data action |
|---|---|---|
| Addressed collision/map/campaign data | `AIBSPTrees`, `AIBinds`, `AIGeometries`, `Buildings`, `Chapters`, `Globals`, `Groups`, `Terrain`, `Units`, `Waypoints` | No new untyped present scenario payload identified. Keep door-234 fallback and absent candidate IDs explicit. |
| Addressed CPU animation/face/effect data | `Animations`, `Binds`, `Effects`, `Heads`, `Locators`, `Sequences`, `Skeletons` | Their corpus/loader tests exist. GPU playback, particles and light output are later stages. |
| Graphics-stage resource use | `Fonts`, `Geometries`, `LRTextures`, `Lights`, `Textures` | Both identified `Geometries` CPU shapes are audited: 6,824 DB-linked model parts and 2,928 construction-piece records (overlapping ID sets). Its 966 package entries without a `CGeometry` DB row have no identified base-game loader key. Display, the other four visual families and animated-light WIP belong to graphics/client stages. |
| Stage-1 media path | `Sounds` | Native audio and its manual game check were handled in stage 1; no new stage-2 world-data decoder is established. |

The sequence gap is now addressed by `NativeHeadSequenceRuntimeTests`'s
optional full DB-linked mode. All 6,780 present `CSequence` payloads pass
strict envelope/track decoding and the game's lazy loader on Windows x64,
Linux x86-64 and ARM64/QEMU, with identical 14,204,108-byte semantic/wire
digest `7F4692A1FFEBAD64`. The source DB also contains 1,579 sequence IDs
without files; 445 are selectable voice-animation links, and the loader
returns no sequence for each rather than crashing. Expression and idle IDs
are all backed. Two package IDs (6006/6007) have no DB row. See
`NATIVE-HEAD-RESOURCES-LINUX.md` for precise values and the separate x86
sampled-frame oracle. These are limits of the original content, not missing
target-platform files to synthesize.

## Final item-2 source-edge and effective-file review (2026-09-28)

The static source audit checked `CResourceOpener`, `CResourceFileOpener`,
`CFileRequest`, share definitions and their `Get` callers in the committed
Windows game and portable source lists. It found the second `Geometries`
shape through building clippers, omitted from the first ledger. The
`NativeBuildingPieceResourceTests` DB-row assertion closes that omission:
Windows x64, Linux GCC x86-64 (ASan/UBSan) and ARM64/QEMU (ASan/UBSan)
all report `construction_rows=885 geometry_ids=814 entries=2928
digest=288D6D988119C2E5 unlinked_package_entries=966`. The separate
model test covers the other game key shape. The 966 entries are *not*
labelled editor-only; they are unreachable through either identified
base-game `Geometries` loader because both keys originate in
`CGeometry` DB pointers. A newly discovered non-DB source edge would
reopen this conclusion.

Effective selection was checked against the base install, not merely the
23 package indexes. Numeric loose directories exist for
`AIBSPTrees`, `AIGeometries`, `Animations`, `Buildings`, `Chapters`,
`Heads`, `Terrain`, `Units`, `Textures`, `Sounds` and `Fonts`; the game
chooses loose before package. The nonvisual typed tests above use the
game opener with that precedence. `Animations` has two loose-only IDs
(2628/2629); `Buildings` and `Terrain` each have loose-only 8021 outside
the 985-variant scenario closure. The `Fonts`, `Textures` and `Sounds`
loose-only files belong to the client graphics or already checked stage-1
media paths. Families without a loose directory resolve to their package.
The 23-family row table, DB/Lua ID joins above and each linked diagnostic
are the coverage register; raw package count alone is never a pass.

Within the agreed stage-2 boundary, no present, game-reachable
nonvisual CPU resource type remains without a typed decoder/test on the
three target architectures. Explicit original-content exceptions remain:
door 234's absent BSP with a checked empty-loader result, missing
optional scenario candidates, 445 selectable voice sequences without a
file, and non-normal-menu campaign records with other shapes. These are
not silently counted as decoded. Dynamic collision/mission parity is
stage 7; display, lighting and particles as imagery are stages 3–4.
This closes the *data-coverage* item, not stage 2 as a whole; the next
gate is the ABI/file-contract audit.

Reproduce the static edge inventory with `rg` over `Main/` for
`CResourceOpener`, `CFileRequest`, `share*.Get`, and the 23 package names.
The authoritative source list for this stage is the committed
`sources.cmake` `Main_SRC`, not all historical files in `Main/`.

# Stage-2 dynamic save-type inventory (3c, diagnostic)

This is a bounded inventory of **observed** object-body type IDs, not a
claim that every registered class or possible runtime state has been
saved. The four pre-existing files below are kept in `G:\SS\lab`; the
tool reads them without modifying them. These are reconstructed-game
snapshots, not a Steam behavioral oracle and not a cross-save product
gate. `ProbeSavedRuntimeType <game.sav> --list-types` prints sorted
`type=<hex> count=<n>` rows and a final distinct-type/body count. Invalid
object-table entries (tombstones) are counted separately and **not**
treated as live instances of their registered type.

| Snapshot under `G:\SS\lab\runs` | SHA-256 of `game.sav` | Live types | Bodies (invalid) | Types unique to this four-file sample |
|---|---|---:|---:|---:|
| `stage2-wire-audit-campaign-20260928-01/user-data/save/default/CAMP_AUDIT` | `E8B43C18E590B22E4952A7C2A00CE484937BCC628F9B1EF8385C61346E4C66A9` | 81 | 4,530 (1,888) | 6 |
| `stage2-wire-audit-voxel-20260928-01/user-data/save/default/VOX_AUDIT` | `F87BBE0D3E7777473C0E58BF7953DFAB66E98EEF2A77DE121738831BE59B043F` | 400 | 27,319 (1,077) | 14 |
| `face-native-manual-20260923-01/game/save/default/stational weapons` | `A16FF62736855A993C227877EF1607085188F5AFB9A5C069010A289D2610483F` | 403 | 28,100 (1,512) | 8 |
| `pre-cutscene-repro-20260923-02/game/save/default/pre_cutscene` | `FA8C1C495A34A53722AAA8CD30204B7640C2589C878BF49E259115652AC47EA7` | 408 | 30,693 (747) | 14 |

The union is 439 live type IDs. The 42 IDs unique to exactly one
file map to the following registered classes (source registrations,
not guesses from names):

| Sample | Unique IDs and classes | 3c decision |
|---|---|---|
| Campaign | `b0912170 CGlobalMapUI`, `b0912172 CZoneGlobalSector`, `b0912180 CGlobalInfoLoader`, `b0912181 CGlobalInfo`, `b1808180 CGlobalMap`, `b2243954 CFlashButton` | `CGlobalInfoLoader`/`CGlobalInfo` are already in the portable core and covered as resource data; the map windows and flash button are client/UI. Do not classify the whole row as visual. |
| Voxel | `00183130 CUICmdExplosionCameraExec`, `013b1140 CActionCounter`, `02443140 CExplosionCube`, `02443141 CExplosionSpace`, `02443142 CIndexCube`, `02443143 C3DLookupTable`, `51653140 CAIAfterCombatLogic`, `51682140 CVoxelExpl`, `52122170 CLUAObjectPosition`, `52782130 CVoxelExplTracker`, `71007380 CUICmdPointCamera`, `a1063160 CMusic`, `b0241948 CEnemyIcon`, `b100711d CStateWait` | Explosion grid/tracker, action counter, AI-after-combat and Lua position are stage-2 CPU candidates; verify which appear in the existing headless explosion save before claiming coverage. Camera/icon/state UI and music are client/media edges. |
| Stationary weapon | `11931180 CAAdder`, `123c1191 CARandom`, `130a1190 CABoneFilter`, `01822162 CUnitStateUsingCannon`, `52443105 CAIRetreatLogic`, `52443130 CAIChoosePlaceForRetreatJob`, `52443140 CAIToPlacePlaceSource`, `52443150 CAIRetreatReaction` | The mounted-weapon state, retreat state and CPU animation graph are reached game classes; require an address-test or justified coverage link, not a Steam gameplay-parity comparison. |
| Pre-cutscene | `01512122 CDGrassEvent`, `02682130 CDynamicPointLight`, `11062160 CLightAnimator`, `11062161 CLightLoader`, `73102121 CLoadTwoBSPTrees`, `a0812160 CAPCritical`, `a11a2141 CUnitGroupAIInfoLoader`, `a1863130 CSound2D`, `a2033140 CPerkButton`, `a2722172 CUnitAIInfoLoader`, `a2722173 CWaypointLoader`, `a2852170 CMETerrainLoader`, `b1202131 CPerksPanelView`, `f0821150 CSWTexture` | `CAPCritical`, AI/waypoint/BSP loaders and terrain data need a CPU coverage decision; grass/light/texture rendering and UI remain deferred to their stages, and sound belongs to the completed stage-1 media route. Preserve the existing animated-light WIP. |

The current strict 52-root tests serialize a party soon after world
initialization, while four other tests save/restore a shot, active slot,
building explosion and inventory-grenade aftermath on Windows x64,
Linux x86-64 and ARM64/QEMU. The four GUI saves had zero raw-type and
unexpected-wire findings in their earlier Windows audits, but those
audits do **not** demonstrate portable reading of every unique runtime
class. In particular, do not equate a test named “explosion” with proof
that it serialized all four explosion-grid classes at the same instant.

The exact `stational weapons` input was copied to the Linux scratch host
with SHA-256 unchanged. On diagnostic Windows x86, Windows x64, Linux
GCC x86-64 (ASan/UBSan/LSan) and ARM64/QEMU (ASan/UBSan), the sorted
403-row live-type listing was byte-identical after newline normalization:
SHA-256 `48494A112F417A00B1924D3AB3176018AA4D82366FF1888A58BDE5C6CABEE577`.
This validates the inventory parser on one real save, not the semantics
of loading that save in a Linux game.

`PortableStructureProbe <raw-world-save> --type-list` now gives the same
sorted type/count view for headless `CWorld` fixtures, which have no GUI
save header. For the six existing mission-810 files in the Windows x64
build directory, the distinct-type counts are: shot 178, direct
explosion 174, grenade 171, grenade-flight 171, inventory-grenade 168,
engineer-inventory-grenade 174. None contains the active-wave IDs
`02443140`–`02443143`, `51682140` or `52782130`: these tests save
*after* the wave has drained. The shot and engineer-grenade snapshots
contain `11931180 CAAdder` and `123c1191 CARandom`; the other candidate
IDs in the table above are absent from all six. Thus an explosion test
name alone must not be cited as coverage of active explosion-state wire.

The direct-explosion fixture SHA-256 is
`AE0AE47C91F04940D96DD9947F08AE617E8B246120D09750EE8FE219497E9E30`.
Its 174-type / 7,470-body listing (33 invalid/tombstone bodies)
has normalized SHA-256
`3FB8AD467BF064F559149169A9D5B557DFDE280DD465D6D97628787DA1D0E1EE`
on diagnostic Windows x86, Windows x64, Linux GCC x86-64 under
ASan/UBSan/LSan, and ARM64/QEMU under ASan/UBSan. Invalid bodies are
counted separately, not assigned to a live class. The same input file
was copied unchanged to the Linux scratch host.

The new diagnostic `NativeWorldInitProbe --mission-party-active-explosion-save
810 <output.sav>` starts four ordinary grenade explosions against the
mission-810 building and stops at the first segment with an in-flight
wavefront. On diagnostic x86, Windows x64, Linux GCC x86-64 under
ASan/UBSan/LSan and ARM64/QEMU under ASan/UBSan this was tick 223, with
four trackers and no queued starts. `CStructureSaver` wrote the active
world, reloaded it, restored runtime caches, and advanced it to an idle
explosion master with changed building HP/voxel hash on each target.
The output file is generated afresh and is not byte-stable even between
two Windows runs; all four type lists contain the same six formerly
missing IDs and counts:

| ID | Count in active save |
|---|---:|
| `02443140 CExplosionCube` | 2 |
| `02443141 CExplosionSpace` | 1 |
| `02443142 CIndexCube` | 2 |
| `02443143 C3DLookupTable` | 1 |
| `51682140 CVoxelExpl` | 1 |
| `52782130 CVoxelExplTracker` | 4 |

Reproduce the bounded check from the game-root working directory after
building both probes from the same source revision:

```text
NativeWorldInitProbe <game.db> <res-directory> --mission-party-active-explosion-save 810 <scratch-active.sav>
PortableStructureProbe <scratch-active.sav> --type-list
```

Use a scratch output path under `lab` (or `/tmp` on Linux), not a user
save. The test returns nonzero if the restored master lacks its active
trackers or does not settle after continued world updates. Inspect the
six `type=` rows above in the second command. On ARM64/QEMU, prefix
both commands with the configured `qemu-aarch64-static -L
/usr/aarch64-linux-gnu` wrapper and disable LeakSanitizer; Linux x86-64
uses ASan/UBSan/LSan.

The active-explosion matrix is complete. Output bodies and final damage
differ between target runs; the gate here is successful active-state
save/load/continuation and six live class IDs, not byte-identical saves.
The mounted-weapon diagnostic creates the real `CUnitStateUsingCannon`
relationship, saves it, reloads it, and verifies that the restored hero is
still in machine-gun state, owns the cannon item, and is the cannon's
current unit. It passed on diagnostic x86, Windows x64, Linux GCC x86-64
with ASan/UBSan, and ARM64/QEMU with ASan/UBSan.
For the remaining CPU classes observed in the stationary-weapon snapshot,
`ProbeSavedRuntimeType <game.sav> <type-id>` produced identical normalized
field-shape hashes on diagnostic x86, Windows x64, Linux x86-64 and
ARM64/QEMU. This covers the four retreat objects (`52443105`, `52443130`,
`52443140`, `52443150`), the animation/position objects (`130a1190`,
`11931180`, `123c1191`), and `CUnitStateUsingCannon` (`01822162`) at the
wire-shape level. The GUI save is not loaded as a Linux game; semantic
runtime behavior remains covered only where a native harness exists.
The retreat diagnostic now creates the real `CAIRetreatLogic` for an
AI-controlled mission unit, saves/reloads the world, and checks that the
restored AI wrapper still owns a `CAIRetreatLogic`. It passed on diagnostic
x86, Windows x64, Linux GCC x86-64 with ASan/UBSan/LSan, and ARM64/QEMU
with ASan/UBSan.
The critical-state diagnostic applies the real `C_AP_REDUCTION` critical
(registered runtime class `CAPCritical`), saves/reloads the world, and
verifies that the restored hero still owns that critical type. It passed on
diagnostic x86, Windows x64, Linux GCC x86-64 with ASan/UBSan/LSan, and
ARM64/QEMU with ASan/UBSan. The subsequent Lua-position check is recorded
below. The remaining CPU-animation records are presentation graph data:
their observed wire shapes agree across targets; playback belongs to the
deferred graphics stage.

The Lua-position diagnostic serializes the actual `CLUAObjectPosition`
payload used by `GetPos`/`GetWaypointPos`, reloads it with the world, and
checks all three coordinates. It passed on diagnostic x86, Windows x64,
Linux GCC x86-64 with ASan/UBSan/LSan, and ARM64/QEMU with ASan/UBSan.
This is about wire/state persistence. Dynamic AI decisions and full
Steam parity remain stage 7.

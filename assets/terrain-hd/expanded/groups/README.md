# Subsequent HD texture groups

These independent lists extend the old world-texture scope. They do **not**
update `sources.json`, the shared `expanded/queue.json`, coverage, calibration,
or the native pack, and they do not claim that the game is fully covered.

`audit_texture_groups.py` uses Python 3.11+ standard-library code to read the
historical columnar database. It resolves serialized table handles to registered
table IDs, checks all 5,801 texture records against `full-texture-inventory.json`,
and records typed texture uses and model/head material consumers. Reproduce with:

```powershell
python assets/terrain-hd/audit_texture_groups.py --database G:/SS/Silent-Storm/Complete/game.db --textures G:/SS/Silent-Storm/Complete/Textures --output assets/terrain-hd/expanded/groups
```

The top-level per-group lists contain every texture, including explicit Bump,
control-only, missing, mixed-role, already accepted and unconfirmed-use records.
`audit-summary.json` contains totals, the source DB/inventory hashes and limitations.
The source-group path selects a list; typed database references establish usage.
Absence of a direct link does not establish that a texture is unused: scripts,
native embedded resources and additional indirect relationships remain to audit.
Particle-instance uses retain their source particle definition, `WrapX/WrapY`,
all nonzero animation-frame bindings, pivots, timing and blending. Additional
SideSize/Tile/frame/row/column/wrap fields are retained when present in a source
row. Animation slot counts do not imply an atlas grid; native particle UVs and
actual source layout still need private inspection before assigning calibration
cells. Missing particle-definition references are explicitly marked for usage review.

Important reference distinctions:

- `Models.Material0`–`Material3`, `Heads.Material0`, building default materials,
  and `Races.MaterialID` reference **MaterialTemplates**. Match them to
  `Materials.TemplateID` before resolving textures. Matching those slots directly
  to `Materials.ID` is incorrect even when same-numbered IDs happen to exist.
- `Materials.TextureID` is diffuse color; `BumpID`, `GlossID` and `MirrorID` are
  control/technical maps. Ordinary DB type alone does not make them artwork.
  Shared color/control IDs are held separately for review.
- `Materials.BRDFID` references BRDF records, not texture IDs.
- `UIControls.TextureN`, icons and cursor `UITexture` reference **UITextures**;
  the latter's resolution fields reference the actual texture.
- Format `565`/native MMP format 8 is RGB565 color. Seven ordinary equipment IDs
  are included: 6197, 6199, 6200, 6247, 6248, 6249, 6250. The UI inventory also
  contains color RGB565 6872, and the miscellaneous list contains 7695.

## Verified originals and source queues

`candidates/*.json` are inputs to the portable `prepare_sources.py`. Technical,
mixed-role, font, cursor and unconfirmed-use records are held in the audit lists.
Missing historical candidates are retained for an explicit fallback status.
For each nonempty list run the following **without `--append`**:

```powershell
python assets/terrain-hd/prepare_sources.py --candidates assets/terrain-hd/expanded/groups/candidates/characters-clothing.json --historical G:/SS/Silent-Storm/Complete/Textures --release G:/SS/lab/baseline/res --output assets/terrain-hd/expanded/groups/source-queues/characters-clothing.json
```

Prepare every nonempty candidate list and the two initial subsets with one
portable command (Python needs Pillow):

```powershell
python assets/terrain-hd/prepare_group_sources.py --historical G:/SS/Silent-Storm/Complete/Textures --release G:/SS/lab/baseline/res --groups assets/terrain-hd/expanded/groups --first-batches
```

The checked results are in `source-queues/*.json`; originals are ordinary PNG
files in `expanded/original`. `source-check-summary.json` records the results.
The verified sets contain 1,803 exact historical/release decoded-RGBA matches
and 15 missing historical sources. No available candidate had a release mismatch,
unsupported native format, or physical storage size differing from DB logical size.
The missing sources are clothing 6137, 6138, 6141, 6142, 6144, 6145, 6146, 7611,
7628, 7629, 7630, 7660, 7661; equipment 614; and interface 7598.

| Group | Verified sources | Missing sources |
| --- | ---: | ---: |
| Characters/clothing | 287 | 13 |
| Equipment | 259 | 1 |
| Weapons | 127 | 0 |
| Heads/LSHead | 186 | 0 |
| Interface v5 | 698 | 1 |
| Effects | 206 | 0 |
| Clues | 17 | 0 |
| Campaign backgrounds | 4 | 0 |
| Final images | 3 | 0 |
| Tutorials | 3 | 0 |
| Miscellaneous | 13 | 0 |

`pending` in these independent source queues means that original RGBA parity
passed; it does **not** mean visual examination, layout validation, imagegen,
brightness calibration or pack validation has passed. Every source must be
privately examined before generation, then each normalized result privately
examined before acceptance. Preserve UVs, atlas cells, silhouette, original alpha,
materials, damage, count/scale of details, character identity and blend mode.
Solid/structural maps discovered during that examination retain their originals.
Duplicate reuse requires exact RGBA, logical size, alpha semantics and layout.

`first-batches/characters-clothing.json` contains 798, 800, 915, 917 (German/UK
soldier Body/Legs). `first-batches/equipment.json` contains 657, 664–669 (knife,
shell bag, backpack, flask, clip case, gas mask, shovel). All eleven have verified
originals and typed material/model consumers; they still need the per-image
source examination and UV/layout review before imagegen. They are independent
inputs for the coordinator after the RGB565 world batch is validated and pushed.

## Interface, cursors and fonts

Original DB `Textures.Width/Height` remain logical texture coordinates. Physical
HD storage can be four times larger; that must not change DB sizes, draw rectangles,
tile boundaries, UI layout, text content, icon state assignments or cursor anchors.
Each `ui_links` entry records the `UITextures` resolution binding, derived logical
UI size, control rectangle and cursor record where available. Dimension derivation
follows `DBFormat/DataInterface.cpp` (1024 mode first, then 1600, 1280, 800).
The unusual `R_1280x960` field uses height divisor 1024 in the current importer;
the audit mirrors that actual behavior instead of guessing from the field name.

`Main/UIWrap.cpp` derives scaling and texture coordinates from original DB sizes.
Before accepting UI artwork check atlas coordinates, borders/stretch regions,
power-of-two padding, readability, exact letters/digits and all button states.
No fonts are sent to imagegen: the 17 Fonts-path resources require a separate
font-rendering audit, and existing vector fonts stay vector fonts.

All available Interface-v5 Cursors-path resources (49) are deferred, including
ArrowOut 7282. The missing cursor 7598 remains an explicit missing-source fallback.
Twenty-five available records have a direct `UICursors -> UITextures -> Textures`
link. The hardware cursor path in `Main/Cursor.cpp:59`–`67` reads original-size
rows from the HD MMP's physical storage without downsampling, producing a corner
of the expanded image. Resolve/check that runtime path before accepting cursor HD.
There is no cursor generation or pack registration in this audit.

Only the coordinator imports approved jobs, adjusts brightness, validates the
entire pack and updates common metadata. This directory contains no generated
artwork, comparisons or HTML. Historical files and release archives were read only.

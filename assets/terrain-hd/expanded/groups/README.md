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

As of 2026-10-08, 365 of these verified originals are accepted in the shared
queue/native pack, and tutorial 6265 is retained as an original structural color
gradient with its original alpha (0–251). All 15 missing historical sources are
explicitly registered as original fallback. The source-queue snapshots stay
unchanged for provenance: consult `../new-groups-twenty-first-batch-check.json` and
`../../sources.json` rather than re-importing these entire lists. The exact
remaining verified set contains 1,380 IDs: clothing 222, equipment 147, weapons
57, heads 184, UI 628, effects 118, clues 17, final 3 and miscellaneous 4.
All four camp atlases and both artistic training targets are now complete.

`first-batches/characters-clothing.json` contains 798, 800, 915, 917 (German/UK
soldier Body/Legs). `first-batches/equipment.json` contains 657, 664–669 (knife,
shell bag, backpack, flask, clip case, gas mask, shovel). All eleven are now
accepted after individual imagegen, UV review, numeric calibration and native
validation. These are provenance inputs and must not be generated again.

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
entire pack and updates common metadata. Artwork and exact prompts are in the
parent expanded/generated directory; there are no public comparisons or HTML.
Historical files and release archives were read only.

The third coordinator packet adds12 magazine atlases and8 status icons. See
`../new-groups-third-batch-check.json` and `../new-groups-third-live-check.json`;
current full pack contains2,031 textures. Remaining counts denote HD acceptance,
not an instruction to regenerate already saved raw artwork in next ready jobs.

The fourth coordinator packet adds10 clothing,12 equipment and12 weapons;
`../new-groups-fourth-batch-check.json` and `../new-groups-fourth-live-check.json`
record the fourth-stage2,065-texture pack.2151/2235 are unaccepted artistic retries
outside shared queue, not permanent refusals. Independent new packets may
already contain raw artwork: inspect current reviews instead of regenerating.

The fifth coordinator packet adds9 clothing,6 equipment,12 weapons and5 UI.
`../new-groups-fifth-batch-check.json` and `../new-groups-fifth-live-check.json`
record the fifth-stage2,097-texture pack and unchanged prior2,065 hashes. Source-queue
snapshots stay unchanged; unaccepted raw/pending reviews are not HD coverage.

The sixth packet accepts2991/2995/2997/3006 revised equipment and700/701/702/704
dust frames. `../new-groups-sixth-batch-check.json` and live report record the
sixth-stage2,105-texture pack with prior2,097 hashes unchanged. Original full alpha
and particle references/blends are retained.2985/2986/597 remain unaccepted.

`new-groups-seventh-batch-check.json` records20 more accepted weapon/UI/effect
textures:2,125 textures/4,250 aliases/114 archives/6,890,090,830 bytes. Prior2,105
hashes unchanged; full source alpha and native particle bindings preserved.
`new-groups-seventh-live-check.json` confirms real-menu HD on/off/on and normal quit.
7199 retains original after a real service rejection;7206 unattempted related hold.
Other pending UI/composite raw artwork is provenance, not accepted HD.

`new-groups-eighth-batch-check.json`:17 clothing,8 weapons and exactduplicate3002
reused from accepted2998 with fresh matching RGBA/dimensions/alphaType/layout.
2,151 textures/4,302 aliases/115 archives/6,930,287,674 bytes; prior2,125 hashes
unchanged and all26 opaque native RGBA equal reviewed calibrated PNG. Real-menu
on/off/on and normal quit recorded in `new-groups-eighth-live-check.json`.
1980 original retained after actual refusal; unaccepted corrections remain provenance.

`../new-groups-ninth-batch-check.json` records12 equipment,10 additive effects,5
weapons:2,178 textures/4,356 aliases/115 archives/6,946,542,070 bytes. Prior2,151
source/PNG/native hashes unchanged. All17 opaque native RGBA equal reviewed PNG;
all10 effects keep original uniformly zero alpha and exact reviewed RGB without
premultiplication. Actual-menu HD on/off/on and normal quit recorded in
`../new-groups-ninth-live-check.json`. Snow5189/5190 and disputed effect/weapon
variants remain unaccepted provenance. Next raw packets require root review.

Tenth accepted packet: `../jobs-new-groups-tenth.json` adds4 clothing,
10 equipment,15 additive effect frames and UI3131 direct-original v2.
2,208 textures/4,416 aliases/116 archives/7,016,536,166 bytes. Prior2,178
source/PNG/native hashes unchanged. All14 opaque native RGBA equal privately
reviewed PNG; all15 effects retain original zero alpha and calibrated RGB
without premultiplication. UI3131 retains exact full original alpha and privately
reviewed premultiplied RGBA. Full validation, actual-menu HD on/off/on and
normal isolated quit: `../new-groups-tenth-{batch,live}-check.json`.
5842 and all8 heads-third attempts held; originals remain. Do not regenerate
accepted IDs. Next independent raw packets require coordinator review.

Eleventh accepted packet: `../jobs-new-groups-eleventh.json` adds6 equipment,5 additive effect frames and UI5842 direct-original v3:2,220 textures/4,440 aliases/117 archives/7,118,248,712 bytes. Prior2,208 source/PNG/native hashes unchanged. Six opaque native RGBA equal reviewed PNG; five effects retain original zero alpha and exact calibrated RGB. UI5842 retains full source alpha, wholecanvas UV and reviewed premultiplied RGBA; localized RGB review resolved the earlier conservative v3 edge-color hold as original brown frame/gold X. v1/v2 remain rejected. Full validation and actual-menu HD on/off/on, normal isolated quit: `../new-groups-eleventh-{batch,live}-check.json`. UI1995 original107x39/native428x156 is held by the packer's NPOT assert; its original stays unchanged. All17 clues-first remain original:16 text/diagram/uncertain and5288 two shifted-rib attempts. Raw/prompt/provenance retained. Exact serviceCRLF prompt writing now avoids Windows CR duplication;12 byte comparisons passed before Git normalization. Do not regenerate accepted IDs.

Twelfth accepted packet: `../jobs-new-groups-twelfth.json` adds6 clothing,10 equipment and3 UI:2,239 textures/4,478 aliases/118 archives/7,176,096,190 bytes. Prior2,220 source/PNG/native hashes unchanged. All17 opaque native RGBA equal privately reviewed calibrated PNG; UI7115/7116 retain complete original alpha and exact premultiplied RGBA. Their source-only wholecanvas nearest affine3:1 helpers inverse-resize the complete raw to original4x, without crop/padding/BBox. Full validation, actual-menu HD on/off/on, normal isolated quit: `../new-groups-twelfth-{batch,live}-check.json`. UI5111 original38x46 is held for NPOT support;5 otherUI,6 clothing and2 equipment attempts remain outside the pack as reviewed. Source/raw/exactprompt provenance retained and initial4010v1 archived before canonicalv2. Do not regenerate accepted IDs.

Thirteenth accepted packet: `../jobs-new-groups-thirteenth.json` adds11 equipment and4 effects:2,254 textures/4,508 aliases/119 archives/7,188,854,696 bytes. Prior2,239 source/PNG/native hashes unchanged. All11 opaque native RGBA equal privately reviewed calibrated PNG;4 additive frames preserve complete original alpha0 and exact calibrated native RGB. Full validation, actual-menu HD on/off/on, normal isolated quit: `../new-groups-thirteenth-{batch,live}-check.json`. Weapons1953/1968/1974 retain originals after actual service refusals, with request IDs/exact arguments/fresh RGBA parity and no retries. Nine other weapon attempts, equipment4858 and8 effect attempts remain reviewed artwork holds, with full raw provenance; they are not accepted HD.

Fourteenth accepted packet: `../jobs-new-groups-fourteenth.json` adds six UI (2940,3958,3959,4049,4438v2,7226) and three miscellaneous artwork maps (1727,7475,7695):2,263 textures/4,526 aliases/123 archives/7,439,814,418 bytes. Prior2,254 source/PNG/native hashes unchanged. Seven opaque native RGBA equal reviewed PNG;2940/4049 retain complete source alpha and exact premultiplied RGBA.2940 uses a wholecanvas source-only affine guide and full inverse without crop/padding/BBox.4438v2 uses direct original builtin generation with quiet original gray fields;v1 archived. Full validation and actual-menu HD on/off/on, normal isolated quit: `../new-groups-fourteenth-{batch,live}-check.json`. UI6658/five other UI remain artwork holds; three Final sky/Earth/cloud maps remain genuine source artwork without generation. Miscellaneous technical candidates await separate disposition. Do not regenerate accepted IDs.

Fifteenth accepted packet: `../jobs-new-groups-fifteenth.json` adds17: two clothing, five equipment, two effects and eight exact duplicates (5455←4012,903←704,904←701,905←702,906←700,2529←5265,4858←2974,4730←2060):2,280 textures/4,560 aliases/123 archives/7,458,165,418 bytes. Prior2,263 source/PNG/native hashes unchanged. All17 native RGBA equal privately reviewed predictions;10 opaque native RGBA equal PNG;7 retain full source alpha. All8 native MMP payloads equal accepted donors byte for byte with zero new imagegen calls; reused raw inputs are donor normalized uncalibrated fullcanvas PNG, with original donor raw/prompt/registration retained. Rejected4858 attempt archived. Full validation and actual-menu HD on/off/on, normal isolated quit: `../new-groups-fifteenth-{batch,live}-check.json`.7658 original retained after actual input service refusal without retry; exact arguments/requestID/refSHA and persistence-time basis recorded. Five structural fills/frames1/3135/3517/5755/7480 and functional font7675 retain original pixels after fresh typed/source parity and full private inspection.5755 is actual zeroRGBA opaque Bedford diffuse, not invented artwork.903/905 unresolved definitions108/139 remain explicitly recorded; identical native reuse changes no DB/UV and does not claim missing blend/Wrap semantics. Do not regenerate accepted IDs.

Sixteenth accepted packet: `../jobs-new-groups-sixteenth.json` adds17: eleven equipment, five paper effects and one clothing atlas:2,297 textures/4,594 aliases/124 archives/7,469,525,932 bytes. Prior2,280 source/PNG/native hashes unchanged. All17 native RGBA equal privately reviewed production4x predictions;12 opaque native RGBA equal PNG,5 effects retain full source alpha and correct premultiplication. Fullcanvas UV/materials/component counts/scale retained, no crop/BBox/artistic sourceRGB repair. Actual-menu HD on/off/on, full native validation and isolated normal quit: `../new-groups-sixteenth-{batch,live}-check.json`. Original native pixel checks resolved provisional1819/1825 topedge concerns: source1819 row0 alreadyblack; source1825 existing topplate y0..3/inlet y4..5 retained.3343 clipped partial redcross and original angular gray mark retained.119 existing approved normalization stages (102 earlier+17new) now ordinary Git PNGs without regeneration/pixel mutation; all declared accepted source stages in Git (`../accepted-git-stage-completeness-sixteenth.json`). Failed independent attempts remain outside pack with provenance; clothing-eleventh0ready. Do not regenerate accepted IDs.

Seventeenth accepted packet: `../jobs-new-groups-seventeenth.json` adds8: three equipment, three effects, one weapon and exact clothing duplicate5453←4010 with zero new imagegen calls. Pack2,305 textures/4,610 aliases/124 archives/7,477,827,592 bytes; prior2,297 source/PNG/native hashes unchanged. All8 nativeRGBA equal privately reviewed production4x predictions; full original alpha, blend/UV/materials/count/scale retained.1936 uses source-only wholecanvas3:1 affine guide/inverse whole4x and explicit source_mask_rgb_padding=False: default padding incorrectly colors alpha-positive RGB-black UV gaps. Full original alpha retained, scalar0.997734, no artistic RGB repair/crop/BBox; defaults and prior accepted assets unchanged. Two meaningful new blackUV regression tests and two existing sourceRGB-region tests pass. Full native validation, actual-menu HD on/off/on, four private screenshots and isolated normal quit/Game absent: `../new-groups-seventeenth-{batch,live}-check.json`. Integration scene confirms layers/menu, not display of every new atlas. All declared accepted stages are ordinary Git files (`../accepted-git-stage-completeness-seventeenth.json`). Nine original16x16 two-color/solid UI5363..5371 retained without generation: actual CFrame UICommCtrls.cpp684..692/UITextures654..662 runtime proof. Genuine artistic UI remain pending.1941 actual OUTPUT moderation refusal/request2cde57e5-5062-4754-ba19-2240dc393004 recorded with exact prompt/refSHA, no retries. Rejected5453v1 archived in generated/rejected-clothing-ninth-5453-v1 before canonical donor reuse. Unaccepted raw stay outside pack with provenance. Do not regenerate accepted IDs.

Eighteenth accepted packet: `../jobs-new-groups-eighteenth.json` adds4 effects1567/1577/5120/5275. Pack2,309 textures/4,618 aliases/124 archives/7,479,225,912 bytes. Prior2,305 source/PNG/native hashes unchanged; all4 nativeRGBA equal privately reviewed production4x predictions. Full original alpha/Transparent premultiply, paper/soft impact materials, original pieces/pattern/wholecanvas UV retained. Existing folds/rims locally more defined and privately accepted at4x; no additional components/creases/hard geometry. Scalar calibration only, no crop/BBox/artistic RGB repair. Full native validation, actual HD on/off/on menu, four private screenshots and isolated normal quit/Game absent: `../new-groups-eighteenth-{batch,live}-check.json`. Integration scene confirms layer/menu, not every individual new effect. Every declared accepted stage remains an ordinary Git file (`../accepted-git-stage-completeness-eighteenth.json`). Unaccepted attempts effects-thirteenth/fourteenth, weapons-tenth/clothing-twelfth retain exact raw/prompts/refSHA/metadata and material/new-detail/blackUV-drift reasons outside pack. Effects-fifteenth0calls, source-only audit. Do not assume1936 padding optout corrects other rejected art; do not regenerate accepted IDs.

Nineteenth accepted packet: seven5425/7209/4651/4661/4642/4723/3044,
six UI backgrounds/materials and one equipment atlas. Pack2,316 textures/4,632
aliases/126 archives/7,599,812,562 bytes. Prior2,309 source/PNG/native hashes
unchanged; all7 nativeRGBA exactly equal privately reviewed production4x predictions.
Wholecanvas UV/sourcealpha/material blend modes retained;3044 keeps exact19 grip
stroke phase. Scalar calibration only, no artistic RGB insertion/crop/BBox.
Eleven genuinely constant or empty fills1881/5393/5394/6644/6646..6652 retain originals;
this does not exclude other CImage or NPOT artwork. GPU UI derives density from
the actual selected file after logical clipping/retail halftexel correction,
without resizing geometry. Physical CopyTexture/ShowTexture, fonts/manual/RT paths
keep their existing units. HD file2D sheets use dedicated textures instead of
overflowing the1024² atlas. Builder accepts exact whole NPOT4x without padding,
retaining the existing short-min mip convention and every old native payload.
Signed-SHORT2 overflow at4096 is fixed by bounded coordinate packing with reciprocal
normalization; full UV is never clamped/cropped, small-texture precision retained.
All84 coordinate/packing checks, two NPOT archive/alpha/mip tests and four existing RGB calibration tests pass.
UI runtime evidence is recorded separately. Full native validation, actual-menu
HD on/off/on and isolated normal quit: `../new-groups-nineteenth-{batch,live}-check.json`.
Integration scene does not certify every new UI/model. All declared accepted
stages are ordinary Git files (`../accepted-git-stage-completeness-nineteenth.json`).
Equipment14/16 and weapons11 held artifacts retain exact provenance;3046/3049
were not imported due to material/local detail drift. Exact accepted-donor audit
of1433 remaining sources against2303 eligible donors (authorized book covers
excluded) found0 matches and made0 new calls. UI13/equipment17 are subsequent
independent packets; inspect saved raw/jobs before generating again.

Twentieth accepted packet: eight textures 609/1995/2034/2200/5111/5843/7455/7664,
two equipment and six interface textures. Pack: 2,324 textures / 4,648 aliases /
126 archives / 7,604,289,534 bytes. All prior 2,316 source/PNG/native
hashes are unchanged. Eight complete nativeRGBA images exactly match privately
reviewed production4x predictions. Full source alpha, UV, materials and pattern
scale remain intact. 1995/5111 reuse saved raw with no regeneration. Whole NPOT
dimensions are neither rounded nor padded. 7455 retains four upper and four lower
cuts; 5843 retains its sole divider at original x504. Accepted detail/contrast
changes are disclosed in ../coordinator-art-review-twentieth.json; exact
local RGB palette identity is not claimed. 2033 remains held: its incorrectly
prompted transparent center and colored edge flecks were not repaired with RGB insertion.
Six structural UI fills 2122..2125/7412/7413 retain originals after individual
whole-source inspection and actual UI binding checks, without excluding other UI/alpha/NPOT art.
Full native validation, actual-menu HD switching and isolated normal quit are
recorded in ../new-groups-twentieth-{batch,live}-check.json. Separate real GPU
5843 on/off/on testing covers whole 2172x164, middle/reverse UV and clipping:
../new-groups-twentieth-ui-runtime-check.json. All frames were privately
reviewed; integration testing does not certify every new element. All declared
PNG/prompt stages are ordinary Git files, ../accepted-git-stage-completeness-twentieth.json.
Held independent attempts preserve exact raw/prompt/reason provenance. Never regenerate accepted IDs.

Twenty-first accepted packet: 5403/6830/5434, one clothing and two equipment
atlases. Pack: 2,327 textures / 4,654 aliases / 126 archives /
7,606,212,086 bytes. All prior 2,324 source/PNG/native hashes are unchanged;
three nativeRGBA images exactly match privately reviewed whole production4x predictions.
Full source alpha, UV, material, component count and pattern scale retained.
Accepted local paint/contrast/warmth changes are disclosed in ../coordinator-art-review-twenty-first.json;
exact local RGB identity is not claimed. No artistic RGB insertion, crop/BBox or
regeneration of accepted IDs. Full native validation, actual-menu HD on/off/on and
isolated normal quit: ../new-groups-twenty-first-{batch,live}-check.json.
All frames privately inspected; this scene does not certify each new model atlas.
All declared PNG/exact-prompt stages are ordinary Git files:
../accepted-git-stage-completeness-twenty-first.json.
Eighteen original-only sources 4282/4362/4363/4784/4806..4817/5115/7464 were
individually inspected in whole RGB/A: sixteen analytical light halos retain exact
native center/ray/falloff profiles; two constant-black RGB Burn_00 images retain
authored alpha scorch shapes and their distinct explosion_decal/overlay/opaque modes.
Zero halo alpha is intentional additive encoding, not an exclusion criterion.
Identical RGBA masks do not authorize material substitution. Missing definition273
for4784 is recorded honestly. Painted particle/alpha art is not generally excluded.
See ../coordinator-original-only-effects-eighteenth.json. Held independent
attempts retain exact raw/prompt/reason provenance.

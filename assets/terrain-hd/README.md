# HD textures

Current user scope (9 October 2026): finish entire heads/hair, clothing and world
categories. Other unfinished categories are paused. One agent owns each whole
category and performs visual and scripted review, with versioned artistic repairs
of unaccepted results. One coordinator owns shared metadata and publication;
build and validate after the category is complete, then publish checked portions.

New job `generation_variant` selects immutable sibling raw, prompt and normalized
filenames; accepted IDs remain excluded. Preserve the exact imagegen argument
followed by one LF. Candidate `source_resource_id` may select the primary ID or
its uncompressed alias (ID | 0x01000000), with an explicit
`source_resource_selection_reason` proving the typed loader selection. Historical
and release resources must match in their complete RGBA. The chosen full RGBA,
including alpha, supplies the reference and calibration; old primary PNGs remain
unchanged. `prepare_sources.py --revisit-originals <IDs>` can reopen individually
reviewed structural-original entries only with a
`reviewed_original_reclassification_reason`; the exact prior disposition is
preserved inside the new entry. Accepted textures cannot be reopened this way.

An individually reviewed job may set `alpha_encoding: source-stored` with
`alpha_encoding_reason` when actual original RGB includes authored contribution
beyond alpha and the material consumes those stored values unchanged. This keeps
the DB texture type and blend mode, restores the complete original alpha, and
avoids an additional RGB multiplication. It is an explicit source/renderer audit,
not an automatic exception for every transparent texture.

For the Russian build/install and authoring instructions, see
[HD-TEXTURES.md](../../HD-TEXTURES.md). To continue generation in a new Codex
session, use [CONTINUE-HD-TEXTURES.md](CONTINUE-HD-TEXTURES.md).

The offline HD pack expands the original five-asset landscape prototype to the
requested terrain, trees, buildings and environment. `expanded/queue.json`
records exact per-resource coverage and processing status; `res-hd/manifest.json`
records what is actually installed. Pending entries are not advertised as HD.
The completed pack contains **2,649 HD textures**: 178 terrain assets, 30 tree
assets, 1,758 building/environment assets, 95 clothing, 145 equipment, 71 weapons,
176 head/hair textures, 103 interface textures, 84 effects, 4 camp atlases, 2 training targets and 3 miscellaneous artwork maps.
All 2,349 world-scope resources have a recorded disposition. The shared queue
contains 3,129 records, including 683 accepted new-group textures, the original
6265 structural gradient and 15 newly recorded missing historical sources.
Original fallback covers 258 technical maps,
141 structural/solid masks, 63 unavailable historical sources, one release mismatch
and seventeen resource IDs whose original-preserving image generation was unavailable
(sixteen distinct source images). No entries in that queue remain pending.
An additional read-only audit found ten ordinary color RGB565 resources excluded
by the old format filter: 5076, 5168, 5169, 6997 and 7581–7586. They are genuine
bark, clock and medical-bed artwork, and all ten exactly match release pixels.
All ten are now accepted, numerically calibrated and fully native-validated,
bringing the queue to 2,349 entries. `expanded/rgb565-followup.json` preserves
the audit and `expanded/rgb565-batch-check.json` records this extension.
Separate character, equipment, weapon, face, interface and effect inventories
are now under `expanded/groups`. They include 1,803 exact release-matched
originals and 15 missing sources. Their source queues are preparation snapshots;
current acceptance is established only by sources.json, shared queue and native
validation. There remain 1,038 verified originals outside the shared queue, including next
raw packets awaiting coordinator review/import/calibration/native validation.
Typed DB usage, deferred control maps/cursors/fonts and reproducible commands
are recorded in `expanded/groups/README.md`. The full 5,801-texture DB
inventory and a ready continuation prompt are in `expanded/full-texture-inventory.json`
and `CONTINUE-HD-TEXTURES.md`; current coverage does not mean all game textures.
The original decoded pixels are matched to the release resources before editing;
historical `Complete/Textures` supplies generation inputs. Steam/release resources
are read only for verification. Missing historical files, release mismatches,
alpha-only shadows, structural masks and solid test overlays retain their native
resources through normal loader fallback.

The initial approved landscape layers are:

| Resource | Layer | Original | Pack |
| --- | --- | --- | --- |
| 1971 | Grass base, four variants | 256×64 | 1024×256 |
| 1970 | Soil base, four variants | 256×64 | 1024×256 |
| 3242 | Grass ground spot | 512×512 | 2048×2048 |
| 6286 | Grass1 ground spot | 512×512 | 2048×2048 |
| 1906 | Grass sprites, 4×4 cells | 256×256 | 1024×1024 |

Replacing the base tiles alone had negligible visual benefit: original ground
spots and grass sprites covered them. The current pack replaces those visible
layers too. Grass is noticeably sharper. The initial generated version was too
bright; the current pack matches historical brightness with a uniform RGB gain
per atlas cell, while retaining approved HD detail, positions and exact alpha.
This remains an art prototype for review.

`*-original.png` preserve decoded sources, `*-generated.png` preserve untouched
image-generation outputs, and `*-atlas.png` / `*-hd.png` are packaged images.
Prompts are in `prompts.md` and `visible-prompts.md`. Generated rasters are
1254×1254; they are resized offline to native power-of-two sizes. The ground
maps' 2048px storage size does not mean 2048px of native AI detail.
Sprite PNGs use straight alpha; the builder premultiplies their colors and
averages premultiplied mip channels for the engine's transparent renderer.

`expanded/new-groups-third-batch-check.json` records 12 additional magazine
atlases and 8 UI status icons, numerical brightness correction, exact native
source alpha, immutable prior2,011 source/PNG/native hashes, and full Game/native
validation of2,031 textures/4,062 aliases/111 archives/6,710,342,282 bytes.
`expanded/new-groups-third-live-check.json` records real-menu HD on/off/on and
normal isolated Game exit. A scene check does not certify every new model/UI state.

`expanded/new-groups-fourth-batch-check.json` records10 clothing,12 equipment
and12 weapon atlases, corrected2155/2350/972 artwork, numerical calibration and
full native validation:2,065 textures/4,130 aliases/112 archives/6,750,015,264 bytes.
All prior2,031 source objects/PNG/native hashes are unchanged; all34 opaque
native top mips match reviewed calibrated PNG RGBA byte for byte. Clothing
2151/2235 stay unaccepted/pending artistic correction; all attempts are saved.
`expanded/new-groups-fourth-live-check.json` records actual-menu HD on/off/on,
2560x1440 and normal isolated Game exit. This is not every-model validation.

## Build and install

`expanded/new-groups-fifth-batch-check.json` records9 clothing,6 equipment,12 weapons
and5 UI icons:2,097 textures/4,194 aliases/112 archives/6,779,639,268 bytes.
All previous2,065 source/PNG/native hashes are unchanged. All27 opaque native top
mips match reviewed calibrated PNG RGBA; all5 native UI top mips were privately
reviewed with the original full alpha. `expanded/new-groups-fifth-live-check.json`
records real-menu HD on/off/on and normal isolated Game exit. Clothing2443/2542/2546,
six initial equipment images, UI6100/6101 and eight heads are pending fidelity
corrections, outside the accepted set; all attempts remain provenance.

`expanded/new-groups-sixth-batch-check.json` records4 corrected equipment
textures and4 full-canvas dust frames:2,105 textures/4,210 aliases/112 archives/
6,781,387,332 bytes. All prior2,097 hashes are unchanged. Full source alpha,
resolved particle animation bindings/blends and original UV canvases are preserved;
no artistic source RGB inserts or whole-atlas registration. Native top mips
privately inspected. `expanded/new-groups-sixth-live-check.json` records actual-menu
HD on/off/on and normal isolated quit.2985/2986 and597 remain unaccepted.

`expanded/new-groups-seventh-batch-check.json` records12 weapons,5 UI textures
and3 fragment frames:2,125 textures/4,250 aliases/114 archives/6,890,090,830 bytes.
All prior2,105 source/PNG/native hashes unchanged; all12 opaque native RGBA and
all8 native original-alpha resources privately inspected.7199 original retained
after actual service refusal;7206 held unattempted. Real-menu on/off/on and normal
isolated quit recorded in `expanded/new-groups-seventh-live-check.json`.

`expanded/new-groups-eighth-batch-check.json` records17 clothing,8 weapons and
exactduplicate3002 reused from accepted2998, with matching fresh source RGBA/dims/
alphaType/layout:2,151 textures/4,302 aliases/115 archives/6,930,287,674 bytes.
Prior2,125 hashes unchanged; all26 opaque native RGBA equal reviewed PNG. Actual-menu
on/off/on and normal isolated quit recorded in `expanded/new-groups-eighth-live-check.json`.
1980 original retained after service refusal; pending artistic corrections unaccepted.

`expanded/new-groups-ninth-batch-check.json` records12 equipment,10 additive
effects and5 weapons:2,178 textures/4,356 aliases/115 archives/6,946,542,070 bytes.
Prior2,151 source/PNG/native hashes unchanged. All17 opaque native RGBA equal
reviewed calibrated PNG; all10 effects retain uniformly zero source alpha and
exact reviewed RGB without premultiplication, original UV and all typed bindings.
Actual-menu HD on/off/on and normal isolated quit recorded in
`expanded/new-groups-ninth-live-check.json`. Disputed raw variants unaccepted.

Tenth accepted packet: `expanded/jobs-new-groups-tenth.json` adds4 clothing,
10 equipment,15 additive effect frames and UI3131 direct-original v2.
2,208 textures/4,416 aliases/116 archives/7,016,536,166 bytes. Prior2,178
source/PNG/native hashes unchanged. All14 opaque native RGBA equal privately
reviewed PNG; all15 effects retain original zero alpha and calibrated RGB
without premultiplication. UI3131 retains exact full original alpha and privately
reviewed premultiplied RGBA. Full validation, actual-menu HD on/off/on and
normal isolated quit: `expanded/new-groups-tenth-{batch,live}-check.json`.
5842 and all8 heads-third attempts held; originals remain. Do not regenerate
accepted IDs. Next independent raw packets require coordinator review.

Eleventh accepted packet: `expanded/jobs-new-groups-eleventh.json` adds6 equipment,5 additive effect frames and UI5842 direct-original v3:2,220 textures/4,440 aliases/117 archives/7,118,248,712 bytes. Prior2,208 source/PNG/native hashes unchanged. Six opaque native RGBA equal reviewed PNG; five effects retain original zero alpha and exact calibrated RGB. UI5842 retains full source alpha, wholecanvas UV and reviewed premultiplied RGBA; localized RGB review resolved the earlier conservative v3 edge-color hold as original brown frame/gold X. v1/v2 remain rejected. Full validation and actual-menu HD on/off/on, normal isolated quit: `expanded/new-groups-eleventh-{batch,live}-check.json`. UI1995 original107x39/native428x156 is held by the packer's NPOT assert; its original stays unchanged. All17 clues-first remain original:16 text/diagram/uncertain and5288 two shifted-rib attempts. Raw/prompt/provenance retained. Exact serviceCRLF prompt writing now avoids Windows CR duplication;12 byte comparisons passed before Git normalization. Do not regenerate accepted IDs.

Twelfth accepted packet: `expanded/jobs-new-groups-twelfth.json` adds6 clothing,10 equipment and3 UI:2,239 textures/4,478 aliases/118 archives/7,176,096,190 bytes. Prior2,220 source/PNG/native hashes unchanged. All17 opaque native RGBA equal privately reviewed calibrated PNG; UI7115/7116 retain complete original alpha and exact premultiplied RGBA. Their source-only wholecanvas nearest affine3:1 helpers inverse-resize the complete raw to original4x, without crop/padding/BBox. Full validation, actual-menu HD on/off/on, normal isolated quit: `expanded/new-groups-twelfth-{batch,live}-check.json`. UI5111 original38x46 is held for NPOT support;5 otherUI,6 clothing and2 equipment attempts remain outside the pack as reviewed. Source/raw/exactprompt provenance retained and initial4010v1 archived before canonicalv2. Do not regenerate accepted IDs.

Thirteenth accepted packet: `expanded/jobs-new-groups-thirteenth.json` adds11 equipment and4 effects:2,254 textures/4,508 aliases/119 archives/7,188,854,696 bytes. Prior2,239 source/PNG/native hashes unchanged. All11 opaque native RGBA equal privately reviewed calibrated PNG;4 additive frames preserve complete original alpha0 and exact calibrated native RGB. Full validation, actual-menu HD on/off/on, normal isolated quit: `expanded/new-groups-thirteenth-{batch,live}-check.json`. Weapons1953/1968/1974 retain originals after actual service refusals, with request IDs/exact arguments/fresh RGBA parity and no retries. Nine other weapon attempts, equipment4858 and8 effect attempts remain reviewed artwork holds, with full raw provenance; they are not accepted HD.

Fourteenth accepted packet: `expanded/jobs-new-groups-fourteenth.json` adds six UI (2940,3958,3959,4049,4438v2,7226) and three miscellaneous artwork maps (1727,7475,7695):2,263 textures/4,526 aliases/123 archives/7,439,814,418 bytes. Prior2,254 source/PNG/native hashes unchanged. Seven opaque native RGBA equal reviewed PNG;2940/4049 retain complete source alpha and exact premultiplied RGBA.2940 uses a wholecanvas source-only affine guide and full inverse without crop/padding/BBox.4438v2 uses direct original builtin generation with quiet original gray fields;v1 archived. Full validation and actual-menu HD on/off/on, normal isolated quit: `expanded/new-groups-fourteenth-{batch,live}-check.json`. UI6658/five other UI remain artwork holds; three Final sky/Earth/cloud maps remain genuine source artwork without generation. Miscellaneous technical candidates await separate disposition. Do not regenerate accepted IDs.

Fifteenth accepted packet: `expanded/jobs-new-groups-fifteenth.json` adds17: two clothing, five equipment, two effects and eight exact duplicates (5455←4012,903←704,904←701,905←702,906←700,2529←5265,4858←2974,4730←2060):2,280 textures/4,560 aliases/123 archives/7,458,165,418 bytes. Prior2,263 source/PNG/native hashes unchanged. All17 native RGBA equal privately reviewed predictions;10 opaque native RGBA equal PNG;7 retain full source alpha. All8 native MMP payloads equal accepted donors byte for byte with zero new imagegen calls; reused raw inputs are donor normalized uncalibrated fullcanvas PNG, with original donor raw/prompt/registration retained. Rejected4858 attempt archived. Full validation and actual-menu HD on/off/on, normal isolated quit: `expanded/new-groups-fifteenth-{batch,live}-check.json`.7658 original retained after actual input service refusal without retry; exact arguments/requestID/refSHA and persistence-time basis recorded. Five structural fills/frames1/3135/3517/5755/7480 and functional font7675 retain original pixels after fresh typed/source parity and full private inspection.5755 is actual zeroRGBA opaque Bedford diffuse, not invented artwork.903/905 unresolved definitions108/139 remain explicitly recorded; identical native reuse changes no DB/UV and does not claim missing blend/Wrap semantics. Do not regenerate accepted IDs.

Sixteenth accepted packet: `expanded/jobs-new-groups-sixteenth.json` adds17: eleven equipment, five paper effects and one clothing atlas:2,297 textures/4,594 aliases/124 archives/7,469,525,932 bytes. Prior2,280 source/PNG/native hashes unchanged. All17 native RGBA equal privately reviewed production4x predictions;12 opaque native RGBA equal PNG,5 effects retain full source alpha and correct premultiplication. Fullcanvas UV/materials/component counts/scale retained, no crop/BBox/artistic sourceRGB repair. Actual-menu HD on/off/on, full native validation and isolated normal quit: `expanded/new-groups-sixteenth-{batch,live}-check.json`. Original native pixel checks resolved provisional1819/1825 topedge concerns: source1819 row0 alreadyblack; source1825 existing topplate y0..3/inlet y4..5 retained.3343 clipped partial redcross and original angular gray mark retained.119 existing approved normalization stages (102 earlier+17new) now ordinary Git PNGs without regeneration/pixel mutation; all declared accepted source stages in Git (`expanded/accepted-git-stage-completeness-sixteenth.json`). Failed independent attempts remain outside pack with provenance; clothing-eleventh0ready. Do not regenerate accepted IDs.

Seventeenth accepted packet: `expanded/jobs-new-groups-seventeenth.json` adds8: three equipment, three effects, one weapon and exact clothing duplicate5453←4010 with zero new imagegen calls. Pack2,305 textures/4,610 aliases/124 archives/7,477,827,592 bytes; prior2,297 source/PNG/native hashes unchanged. All8 nativeRGBA equal privately reviewed production4x predictions; full original alpha, blend/UV/materials/count/scale retained.1936 uses source-only wholecanvas3:1 affine guide/inverse whole4x and explicit source_mask_rgb_padding=False: default padding incorrectly colors alpha-positive RGB-black UV gaps. Full original alpha retained, scalar0.997734, no artistic RGB repair/crop/BBox; defaults and prior accepted assets unchanged. Two meaningful new blackUV regression tests and two existing sourceRGB-region tests pass. Full native validation, actual-menu HD on/off/on, four private screenshots and isolated normal quit/Game absent: `expanded/new-groups-seventeenth-{batch,live}-check.json`. Integration scene confirms layers/menu, not display of every new atlas. All declared accepted stages are ordinary Git files (`expanded/accepted-git-stage-completeness-seventeenth.json`). Nine original16x16 two-color/solid UI5363..5371 retained without generation: actual CFrame UICommCtrls.cpp684..692/UITextures654..662 runtime proof. Genuine artistic UI remain pending.1941 actual OUTPUT moderation refusal/request2cde57e5-5062-4754-ba19-2240dc393004 recorded with exact prompt/refSHA, no retries. Rejected5453v1 archived in generated/rejected-clothing-ninth-5453-v1 before canonical donor reuse. Unaccepted raw stay outside pack with provenance. Do not regenerate accepted IDs.

Eighteenth accepted packet: `expanded/jobs-new-groups-eighteenth.json` adds4 effects1567/1577/5120/5275. Pack2,309 textures/4,618 aliases/124 archives/7,479,225,912 bytes. Prior2,305 source/PNG/native hashes unchanged; all4 nativeRGBA equal privately reviewed production4x predictions. Full original alpha/Transparent premultiply, paper/soft impact materials, original pieces/pattern/wholecanvas UV retained. Existing folds/rims locally more defined and privately accepted at4x; no additional components/creases/hard geometry. Scalar calibration only, no crop/BBox/artistic RGB repair. Full native validation, actual HD on/off/on menu, four private screenshots and isolated normal quit/Game absent: `expanded/new-groups-eighteenth-{batch,live}-check.json`. Integration scene confirms layer/menu, not every individual new effect. Every declared accepted stage remains an ordinary Git file (`expanded/accepted-git-stage-completeness-eighteenth.json`). Unaccepted attempts effects-thirteenth/fourteenth, weapons-tenth/clothing-twelfth retain exact raw/prompts/refSHA/metadata and material/new-detail/blackUV-drift reasons outside pack. Effects-fifteenth0calls, source-only audit. Do not assume1936 padding optout corrects other rejected art; do not regenerate accepted IDs.

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
HD on/off/on and isolated normal quit: `expanded/new-groups-nineteenth-{batch,live}-check.json`.
Integration scene does not certify every new UI/model. All declared accepted
stages are ordinary Git files (`expanded/accepted-git-stage-completeness-nineteenth.json`).
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
changes are disclosed in expanded/coordinator-art-review-twentieth.json; exact
local RGB palette identity is not claimed. 2033 remains held: its incorrectly
prompted transparent center and colored edge flecks were not repaired with RGB insertion.
Six structural UI fills 2122..2125/7412/7413 retain originals after individual
whole-source inspection and actual UI binding checks, without excluding other UI/alpha/NPOT art.
Full native validation, actual-menu HD switching and isolated normal quit are
recorded in expanded/new-groups-twentieth-{batch,live}-check.json. Separate real GPU
5843 on/off/on testing covers whole 2172x164, middle/reverse UV and clipping:
expanded/new-groups-twentieth-ui-runtime-check.json. All frames were privately
reviewed; integration testing does not certify every new element. All declared
PNG/prompt stages are ordinary Git files, expanded/accepted-git-stage-completeness-twentieth.json.
Held independent attempts preserve exact raw/prompt/reason provenance. Never regenerate accepted IDs.

Twenty-first accepted packet: 5403/6830/5434, one clothing and two equipment
atlases. Pack: 2,327 textures / 4,654 aliases / 126 archives /
7,606,212,086 bytes. All prior 2,324 source/PNG/native hashes are unchanged;
three nativeRGBA images exactly match privately reviewed whole production4x predictions.
Full source alpha, UV, material, component count and pattern scale retained.
Accepted local paint/contrast/warmth changes are disclosed in expanded/coordinator-art-review-twenty-first.json;
exact local RGB identity is not claimed. No artistic RGB insertion, crop/BBox or
regeneration of accepted IDs. Full native validation, actual-menu HD on/off/on and
isolated normal quit: expanded/new-groups-twenty-first-{batch,live}-check.json.
All frames privately inspected; this scene does not certify each new model atlas.
All declared PNG/exact-prompt stages are ordinary Git files:
expanded/accepted-git-stage-completeness-twenty-first.json.
Eighteen original-only sources 4282/4362/4363/4784/4806..4817/5115/7464 were
individually inspected in whole RGB/A: sixteen analytical light halos retain exact
native center/ray/falloff profiles; two constant-black RGB Burn_00 images retain
authored alpha scorch shapes and their distinct explosion_decal/overlay/opaque modes.
Zero halo alpha is intentional additive encoding, not an exclusion criterion.
Identical RGBA masks do not authorize material substitution. Missing definition273
for4784 is recorded honestly. Painted particle/alpha art is not generally excluded.
See expanded/coordinator-original-only-effects-eighteenth.json. Held independent
attempts retain exact raw/prompt/reason provenance.

Twenty-second accepted packet: 1746/2447/6831/1997/3974, two equipment,
one clothing and two interface backgrounds. Pack: 2,332 textures / 4,664 aliases /
126 archives / 7,612,401,150 bytes. All prior 2,327 source/PNG/native
hashes unchanged; five nativeRGBA images exactly match privately reviewed whole
production4x predictions. Full original alpha/UV/material/field count/pattern
scale retained; existing edge/paint/warmth refinements disclosed in
expanded/coordinator-art-review-twenty-second.json without claiming exact localRGB identity.
1997/3974 retain entire NPOT255x130→1020x520 and103x104→412x416 without padding,
rounding/crop/BBox. Only scalar numeric brightness, no artistic RGB insertion.
ActualGPU on/off/on for both UI covers whole/middle/reversedUV/clipping:
expanded/new-groups-twenty-second-ui-runtime-check.json. Full native validation,
real-menu HD switching, all frames privately inspected and isolated normal quit:
expanded/new-groups-twenty-second-{batch,live}-check.json. Integration scene does
not certify every new model atlas. Declared PNG/exact-prompt stages are ordinary
Git files: expanded/accepted-git-stage-completeness-twenty-second.json.
6832/6815 remain held for palette and local shape/UV drift; exact raw/prompt/proofs
retained. Never regenerate accepted IDs; inspect current independent jobs/reviews
before new calls.

Twenty-third accepted packet: 5376/5432/5648/3321, two equipment,
one clothing and one hair diffuse texture. Pack: 2,336 textures / 4,672 aliases /
126 archives / 7,615,896,618 bytes. All prior 2,332 source/PNG/native
hashes unchanged; four nativeRGBA images exactly match privately reviewed whole
production4x predictions. Full source alpha/UV/material/count/pattern scale
preserved:16 lowerleft bands5376,15 lines/two diagonal straps5648,two main hair
lobes/central part3321. Existing paint/marks/strand refinement and moderate local
peak lifts disclosed in expanded/coordinator-art-review-twenty-third.json, without
claiming exact localRGB identity. Scalar brightness only, no artisticRGB insertion,
crop/BBox or accepted regeneration. Full native validation, real-menu HD on/off/on,
all frames privately viewed and isolated normal quit:
expanded/new-groups-twenty-third-{batch,live}-check.json. Scene certifies layer/menu,
not every new model. All declared PNG/prompt stages are ordinary Git:
expanded/accepted-git-stage-completeness-twenty-third.json. Exact original worker
reviews frozen separately; later ancillary additions preserve reviewed artwork.
Canonical imported prompt is exact actual argument+one serviceLF:
expanded/coordinator-review-provenance-twenty-third.json.

1998 retains exact original constantblack Slot Fade/authoredA0..128 with21 alpha
levels/rounded corners, actualUI348/controls975/1029 after wholeRGB/A review:
expanded/coordinator-original-only-ui-nineteenth.json. No globalUI/alpha/NPOT
exclusion. Remaining1,370 exact source metadata screened against2,328 eligible
accepted donors with sourceRGBA/dimensions/nativeType:0 matches/0calls, authorized
book-cover content replacements excluded. 1822/6239/5666/3319/5740 remain held,
raw/exactprompts/proof retained. Inspect subsequent jobs/reviews before new calls.

Twenty-fourth accepted packet: equipment5406/2448/3051, hair6769/6770/6765/6767,
neck/hands color6761 and stone-menu4653. Pack:2,345 textures/4,690 aliases/
126 archives/7,629,266,454 bytes. All prior2,336 source/PNG/native
hashes unchanged; nine nativeRGBA images exactly match privately viewed whole4x
predictions. Full originalalpha/UV/material/count/pattern scale retained. 5406
retains8 square alpha-zero gaps, one large rounded gap and every sourceAA level.
Hair flow/part retained; moderate existing paint peak lifts disclosed in
expanded/coordinator-art-review-twenty-fourth.json. 6761 retains3 original soft
washes/neck-hands color with disclosed weak digital flatfield variation, no new
anatomy/grain. Scalar brightness only; no artisticRGB insertion/crop/BBox.
Actual4653 UI sampler HD on/off/on, real graphics menu and isolated normalquit:
expanded/new-groups-twenty-fourth-{batch,live}-check.json. Frames privately viewed;
scene certifies layer/menu, not every model. Ordinary Git PNG/prompt stages and
frozen original worker reviews:expanded/accepted-git-stage-completeness-twenty-fourth.json,
expanded/coordinator-review-provenance-twenty-fourth.json. Held5405/5388/3041/6768/
6766/7627/3864/2153/6827 retain raw/reasons. Inspect next jobs before newcalls.

Twenty-fifth accepted packet: equipment6403/7659, neck/hands color6762
and dynamic FaceGen hair layer6715. Pack:2,349 textures/4,698 aliases/
126 archives/7,632,936,686 bytes. Prior2,345 source/PNG/native hashes unchanged;
four nativeRGBA images equal privately reviewed whole4x predictions. Full source
alpha/UV/material/scale preserved, numerical brightness only, no artisticRGB
insertion/crop/BBox. Source2378 individually retained:4096 identical RGBA68/52/35/255
pixels, zero generation; no equipment category exclusion.
FaceGen now reads the complete logical mip of HD layers, retaining retail
straight-alpha blending/Vflip and256square atlas. Live preview invalidates on
HD revision. New23 regression checks and4 selected CTest pass. Actual6715
resource loader/CPU blend on/off/on matches independent integer/FNV references;
9 real head94 bakes/roundtrips pass. This does not certify every head on GPU.
Full native validation, actual graphics menu/private frames/normalquit:
expanded/new-groups-twenty-fifth-{batch,live}-check.json; actual CPU proof:
expanded/new-groups-twenty-fifth-facegen-runtime-check.json. Three original worker
reviews frozen before import; exact prompts and PNG remain ordinary Git files.
Held candidates retain reasons; do not regenerate accepted IDs.

Twenty-sixth accepted packet: equipment5437/5436, hair6771 and dynamic
face-color layers6671/6678. Pack:2,354 textures/4,708 aliases/127 archives/
7,638,529,374 bytes. Prior2,349 source/PNG/native hashes unchanged; five nativeRGBA
equal privately viewed whole4x predictions. Full sourcealpha/UV/material/count/
scale retained. Faces preserve originalidentity/closedlips/softpaint, no teeth,
new anatomy or photopores. Moderate localpalette/contrast changes disclosed in
expanded/coordinator-art-review-twenty-sixth.json; numerical brightness only.
Actual6671/6678 resource-loader/CPU256atlas on/off/on matches independent integer
alpha/Vflip/FNV references;9 realhead94 bakes/roundtrips passed. GameEXE and code
unchanged since verified25, no fullheadGPU claim. Full nativevalidation, actual
menu/four privately viewed frames/normalquit: expanded/new-groups-twenty-sixth-
{batch,live}-check.json; actualCPU proof: expanded/new-groups-twenty-sixth-
facegen-runtime-check.json. OrdinaryGit PNG/exactprompts and2 frozen workerreviews.
Inspect next independentjobs before imagegen; do not regenerate accepted IDs.

Twenty-seventh accepted packet: equipment2984/3000, clothing7614/1869,
dynamic face-color6679, eye6672 and UI4655. Pack:2,361 textures/4,722 aliases/
127 archives/7,657,054,604 bytes. Prior2,354 source/PNG/native hashes unchanged;
seven nativeRGBA equal privately viewed whole4x predictions. Full sourcealpha,
UV/material/count/scale retained. All1,008 calibrated7614 checker centres preserve
sourcephase. Local painted-tone differences disclosed in expanded/coordinator-art-
review-twenty-seventh.json; numerical brightness only, no artisticRGB inserts.
Actual6679/6672 SWloader/CPU blending on/off/on matches independent integeralpha/
Vflip/FNV references; fixed eye diagnostic quadrant does not certify GPU placement.
Nine actualhead94 bakes/roundtrips passed. Actual4655 GPU whole/middle/reverse/
clipped draws privately inspected on/off/on. GameEXE/code unchanged since verified25.
Full nativevalidation/realmenu/four privateframes/normalquit: expanded/new-groups-
twenty-seventh-{batch,live}-check.json; actuallayer/UI proof: expanded/new-groups-
twenty-seventh-runtime-check.json; expanded/coordinator-pattern-check-twenty-seventh.json.
OrdinaryGit PNG/exactprompts and4 frozen workerreviews. Inspect next readyjobs
before imagegen; do not regenerate accepted IDs.

Twenty-eighth accepted packet: equipment676/2968/4257/1821/1827,
clothing5469/7590, dynamic eye6686, UI4659 and exact2932 reuse of accepted1869.
Nine separate builtin generations, one reused result with no new call.
Pack:2,371 textures/4,742 aliases/127 archives/7,679,774,288 bytes.
Prior2,361 source/PNG/native hashes unchanged; all10 nativeRGBA equal privately
viewed whole4x predictions. Full sourcealpha/UV/material/count/scale retained;
localpaint changes disclosed in expanded/coordinator-art-review-twenty-eighth.json.
Only numericalbrightness, no artisticRGB inserts. Two4659 wall repeats retain
sourcephase; modest finepaint differences betweenhalves disclosed, no forced tilecopy.
2932/1869 fullsourceRGBA/dimensions/alphatype/layout match, productionnative2932
equals accepted1869 byteexact: expanded/coordinator-reuse-check-twenty-eighth.json.
Actual6686 SWloader/CPU blending on/off/on matches independent integeralpha/Vflip/FNV;
nine actualhead94 bakes/roundtrips passed. Fixed diagnostic quadrant does not
certify eyeGPU placement. Actual4659 GPU whole/middle/reverse/clipped draws reviewed.
GameEXE/code unchanged since verified25. Full nativevalidation/realmenu/fourprivate
frames/normalquit: expanded/new-groups-twenty-eighth-batch-check.json and
expanded/new-groups-twenty-eighth-live-check.json; eye/UI integration:
expanded/new-groups-twenty-eighth-runtime-check.json. Six workerreviews frozen
beforeimport, ordinaryGit PNG/exactprompts. Check next readyjobs before imagegen.

Twenty-ninth accepted packet: equipment5389/5404, clothing4571/4561/4563,
UI5203 and exact3255/4066 reuse of accepted1869/6761. Six separate builtin
generations, two reused results with no new calls. Pack:2,379 textures/4,758
aliases/128 archives/7,736,048,308 bytes. Prior2,371 source/PNG/native hashes
unchanged; all8 nativeRGBA equal privately viewed whole4x predictions. Full
sourcealpha/UV/material/count/scale retained, localpaint changes disclosed in
expanded/coordinator-art-review-twenty-ninth.json. Only numericalbrightness,
no artisticRGB inserts. Duplicate productionpayloads equal accepted donors:
expanded/coordinator-reuse-check-twenty-ninth.json. Deliberately blurred5203
reviewed in actual GPU whole/middle/reverse/clipped draws on/off/on. UITextures638
registration is proven; active-widget binding remains unconfirmed. This certifies
the generic sampler. GameEXE/six codefile hashes unchanged since verified25.
Full nativevalidation/realmenu/four privateframes/normalquit:
expanded/new-groups-twenty-ninth-batch-check.json and
expanded/new-groups-twenty-ninth-live-check.json; GPU integration:
expanded/new-groups-twenty-ninth-runtime-check.json. Three worker reviews frozen
before import. OrdinaryGit PNG; accepted IDs must not be regenerated.

Thirtieth accepted packet: clothing5482/5621, dynamic eyes6688/6690,
Storepistols5018. Five separate builtin generations. Pack:2,384 textures/4,768
aliases/128 archives/7,742,536,642 bytes. Prior2,379 source/PNG/native hashes unchanged;
all5 nativeRGBA equal privately viewed whole4x predictions. Fullsourcealpha/UV/
material/count/scale retained;5482 stripes17/16/21 pitch4 and5621 fivepairedglints
pitch10 retained. Localpaint/reflection changes disclosed in
expanded/coordinator-art-review-thirtieth.json. Numericalbrightness only,
no artisticRGB insertion. Complete5018 originalalpha restored/premultiplyonce.
Actual6688/6690 wholelogicalSWsource/CPUcompositor on/off/on hashes match
independent integeralpha/Vflip/FNV;9actualhead94 bakes/roundtrips passed.
Fixedface diagnostic quadrant does not certify actualGPUeye placement.
Actual5018 GPU whole/middle/reverse/clipped draws andatlasplacement reviewed.
UI620/iStorePanel367binding proven; actualStorepanel opening notclaimed.
GameEXE/six codefile hashes unchanged since verified25. Fullnativevalidation/
realmenu/fourprivateframes/normalquit: expanded/new-groups-thirtieth-batch-check.json
and expanded/new-groups-thirtieth-live-check.json; eye/GPU integration:
expanded/new-groups-thirtieth-runtime-check.json. Three worker reviews frozen
beforeimport. OrdinaryGit PNG; accepted IDs must not be regenerated.

Thirty-first accepted packet: equipment6473, clothing5616/4517/4685,
flag maps4749/4750. Six separate builtin generations. Pack:2,390 textures/4,780
aliases/129 archives/7,774,693,306 bytes. Prior2,384 source/PNG/native hashes unchanged;
all6 nativeRGBA equal privately viewed whole4x predictions. Full4517 sourcealpha,
UV/material/count/scale retained; localpaint variations disclosed in
expanded/coordinator-art-review-thirty-first.json. Numericalbrightness only,
no artisticRGB inserts. Actual4750 GPU whole/middle/reverse/clipped draws on/off/on
reviewed; material/model binding proven, complete3Dflag appearance notcertified.
GameEXE/code unchanged since verified25. Nativevalidation/realmenu/fourprivateframes/
normalquit: expanded/new-groups-thirty-first-batch-check.json,
expanded/new-groups-thirty-first-live-check.json and
expanded/new-groups-thirty-first-runtime-check.json. Three reviews frozen beforeimport;
ordinaryGit PNG, accepted IDs must not be regenerated.

Thirty-second accepted packet: clothing3888/6430, UI5799/5781/5829/5831/5800/5823/5835,
paperfragments1578. Ten separate builtin generations. Pack:2,400 textures/4,800
aliases/129 archives/7,783,410,100 bytes. Prior2,390 source/PNG/native hashes unchanged;
all10 nativeRGBA equal privately viewed whole4x predictions. Fullsourcealpha,
UV/material/count/paintscale retained; localtone/softedge variations disclosed in
expanded/coordinator-art-review-thirty-second.json. Numericalbrightness only,
no artisticRGB inserts. Actual5831/1578 GPU whole/middle/reverse/clipped on/off/on
and atlasplacement reviewed; no activeperkpanel or actualeffectscene appearance claim.
1086 held for source-relative orange paint hierarchy; no retry. GameEXE/code
unchangedverified25. Nativevalidation/realmenu/fourprivateframes/normalquit:
expanded/new-groups-thirty-second-batch-check.json,
expanded/new-groups-thirty-second-live-check.json and
expanded/new-groups-thirty-second-runtime-check.json. Six reviews frozen beforeimport;
ordinaryGit PNG, accepted IDs must not be regenerated.

Thirty-third accepted packet: equipment4867/5676/7653, clothing4544,
UI2089/2093/2257. Seven separate builtin generations. Pack:2,407 textures/4,814
aliases/129 archives/7,786,446,964 bytes. Prior2,400 source/PNG/native hashes unchanged;
all7 nativeRGBA equal privately viewed whole4x predictions. Fullsourcealpha,
UV/material/count/paintscale retained; individual localpaint changes disclosed in
expanded/coordinator-art-review-thirty-third.json. Numericalbrightness only,
no artisticRGB inserts. Actual2093 GPU whole/middle/reverse/clipped on/off/on
and atlasplacement reviewed; UI388 FirstAid consumer proven, actual activepanel
appearance not certified. GameEXE/code unchangedverified25. Nativevalidation/
realmenu/fourprivateframes/normalquit: expanded/new-groups-thirty-third-batch-check.json,
expanded/new-groups-thirty-third-live-check.json and
expanded/new-groups-thirty-third-runtime-check.json. Four reviews frozen beforeimport;
ordinaryGit PNG, accepted IDs must not be regenerated.

Thirty-fourth accepted packet: static eye1126/hair3324, dynamic face6710/6711,
clothing4453/6825 and icon5811. Seven separate builtin generations. Pack:2,414
textures/4,828 aliases/129 archives/7,795,382,080 bytes. Prior2,407 source/PNG/native
hashes unchanged; all7 nativeRGBA equal privately viewed whole4x predictions.
Fullsourcealpha/UV/material/count/paintscale retained; localpaint variations
disclosed in expanded/coordinator-art-review-thirty-fourth.json.1126 weakpigment
already exists in source; separate honest postcall correction retains original
prompt/before-call proof. Numericalbrightness only, no artisticRGB inserts.
Actual1126/5811 GPU whole/middle/reverse/clipped on/off/on and atlasplacement
reviewed.1126 uses staticMaterials745/Template520, notTHMID eye. Actual6710/6711
128face SWloader/mip2/fullsourcealpha/Vflip/independentinteger CPU hashes and
nine head94bakes/static/textured/roundtrips verified; fixed256faceatlas unchanged.
No completeGPU head or activeperkpanel appearance claim. GameEXE/code unchanged25.
Nativevalidation/realmenu/fourprivateframes/normalquit:
expanded/new-groups-thirty-fourth-batch-check.json,
expanded/new-groups-thirty-fourth-live-check.json and
expanded/new-groups-thirty-fourth-runtime-check.json. Four reviews frozen beforeimport;
ordinaryGit PNG, accepted IDs must not be regenerated.

Thirty-fifth accepted packet: effect1078/clothing6826/equipment5390 and
UI5846/5847/5880. Six separate builtin generations. Pack:2,420 textures/4,840
aliases/129 archives/7,797,295,132 bytes. Prior2,414 source/PNG/native hashes unchanged;
all6 nativeRGBA equal privately reviewed whole4x predictions. Whole sourcealpha,
UV/material/count/paintscale retained; localpaint variations disclosed in
expanded/coordinator-art-review-thirty-fifth.json. Numericalbrightness only,
no artisticRGB inserts. TransparentAdd1078 retains usefulRGB with wholeA0,
no premultiply. Actual5846/1078 GPU whole/middle/reverse/clipped on/off/on,
NPOT35→140 and atlasplacement reviewed. UI700/702 registration-only; UI759
iCluesMenu.cpp220 consumer proven. All ParticleInstances TextureN1078 audited;
absentParticles153/163 explicitly unknown. Activeeffect/widget appearance and
absentWrap/draw rules notcertified. NoFaceGen claim. Code unchanged25;
current built GameEXE707a588b used, fullSHA in build-provenance report.
Nativevalidation/realmenu/fourprivateframes/normalquit:
expanded/new-groups-thirty-fifth-batch-check.json,
expanded/new-groups-thirty-fifth-live-check.json and
expanded/new-groups-thirty-fifth-runtime-check.json. Five reviews frozen beforeimport;
held1937/6822 preserved without retry; ordinaryGit PNG/RESbuiltwithGame.

Thirty-sixth accepted packet: hairlayer6720/equipment5238/5409 and
UI5844/5845/5802/5803/5784/5814, nine separate builtin generations.
Pack2,429 textures/4,858 aliases/129 archives/7,800,387,976 bytes.
Prior2,420 source/PNG/native hashes unchanged. All9 actualnativeRGBA equal
privately reviewed4x predictions; fullsourcealpha/UV/material/count/paintscale
retained, numericalbrightness only. Localpaint caveats disclosed in
expanded/coordinator-art-review-thirty-sixth.json. ActualGPU5844/5802
whole/middle/reverse/clipped on/off/on; NPOT35to140/atlasplacement correct.
UI699/701 registration-only; disabledperk IconDisabled bindings through
RPGPerks/DataPerk/iPerksPanel proven. Actual6720 logical128 SWmip2/fullBGRA/
Vflip/independent integeralpha/FNV passed three states. Actualhead94
static/textured/roundtrip ninebakes passed/restoredHDhashes exact.
Pipeline proof does not certify completeGPUhead or activewidget appearance.
Code unchanged25; currentbuiltGame707a588b used. Nativevalidation/realmenu
11/0/11/fourprivateframes/normalquit: expanded/new-groups-thirty-sixth-batch-check.json,
expanded/new-groups-thirty-sixth-live-check.json and expanded/new-groups-thirty-sixth-runtime-check.json.
Seven reviews frozen; held variants preserved without retries; ordinaryGitPNG/RESbuiltwithGame.

Thirty-seventh accepted packet: clothing3849/7631/5615/UI5826/5801/5804/effect5261.
Seven separate builtin generations. Pack2,436 textures/4,872 aliases/129 archives/
7,813,582,920 bytes. Prior2,429 source/PNG/native hashes unchanged. All7 actualnativeRGBA
equal privately reviewed whole4x predictions/fullsourceA/UV/material/count/paintscale.
Source7631 lowergray boundary/5615 lowerangledgrayfold already exist, no newseam/thirdbutton.
Localpaint caveats disclosed in expanded/coordinator-art-review-thirty-seventh.json;
numericalbrightness only/no artisticRGB inserts. ActualGPU5826/5261 whole/middle/reverse/
clipped on/off/on/placement/density4vs1 reviewed; TransparentAdd5261 keeps usefulRGB withA0,
no premultiply. UI742 registration-only;UI717/720 RPGPerks14/DataPerk/iPerksPanel proven.
All ParticleInstances TextureN5261 audited; missing siblings4737/4738 explicitunknown.
Activeeffect/widget/full3Dmodel appearance notcertified; noFaceGen37. Code unchanged25,
currentbuiltEXE707a588b byteexact to testedGame. Nativevalidation/realmenu11/0/11/fourprivateframes/
normalquit: expanded/new-groups-thirty-seventh-batch-check.json,
expanded/new-groups-thirty-seventh-live-check.json and expanded/new-groups-thirty-seventh-runtime-check.json.
Five frozenreviews/held2118and5838/no retries; ordinaryGitPNG/RESbuiltwithGame.

Thirty-eighth accepted packet: effects4095/2577 and UI5808/5749.
Four separate builtin generations. Pack2,440 textures/4,880 aliases/129 archives/
7,814,413,248 bytes. Prior2,436 source/PNG/native hashes unchanged. All4 actualnativeRGBA
equal privately reviewed whole4x predictions/fullsourceA/UV/material/count/paintscale.
5808 retains EIGHT irregular sourcepaintpeaks/weaksidebands;5749 retains THREE old components.
4095/2577 oldsoftpaintfields/darkpockets and blackgaps retained. Localpaint caveats disclosed
in expanded/coordinator-art-review-thirty-eighth.json; numericalbrightness only/no RGBinserts.
ActualGPU5808/2577/4095 whole/middle/reverse/clipped on/off/on/placement/density4vs1,
ninewholeprivateframes reviewed; TransparentAdd2577 keeps usefulRGB withA0/noPremul,
Transparent4095 fullsourceA0..255/premulonce. UI724 RPGPerks51/52 IconDisabled/DataPerk23/
iPerksPanel67/68 proven;UI687 registration-only. All ParticleInstances TextureN4095/2577
audited; missingdefinitions/siblings explicitunknown. Activeeffect/widget/full3Dmodel appearance
notcertified/noFaceGen38. Code unchanged25/currentbuiltEXE707a588b byteexact to testedGame.
Nativevalidation/realmenu11/0/11/fourprivateframes/normalquit:
expanded/new-groups-thirty-eighth-batch-check.json, expanded/new-groups-thirty-eighth-live-check.json
and expanded/new-groups-thirty-eighth-runtime-check.json. Six frozenreviews;rootheld7634
retains originalcounts butsoftmarks becamebrightbeadlike/newbrownline appeared;no retries/repair.
OrdinaryGitPNG/RESbuiltwithGame.

Thirty-ninth accepted packet: effects4094/4096, UI5948, facecolorlayers6706/6707.
Five separate builtin generations. Pack2,445 textures/4,890 aliases/129 archives/
7,817,521,008 bytes. Prior2,440 source/PNG/native hashes unchanged. All5 actualnativeRGBA
equal privately reviewed whole4x predictions/fullsourceA/UV/material/count/nativepaintscale.
6706/6707 ONE/TWO softdiagonal traces and oldauthoringcorners/fullhiddenRGB retained;
redcenterstrength/localpaint caveats disclosed in expanded/coordinator-art-review-thirty-ninth.json.
5948 keeps botholdinnerdarkwashes;4094/4096 keepsoftgraybrownpaintfields.
4096 promptclippingdescription error separatelyrecorded/fullnativebeforematrix alwayscorrect:
column31RGBA0,tailx30. Three individual wholezeroRGBA6777/6778/6779 originalretained/0calls,
expanded/coordinator-original-only-heads-seventeenth.json. No categoryexclusion/no RGBrepair/retries.
ActualGPU5948/4094/4096 whole/middle/reverse/clipped on/off/on/placement/density4vs1/
fullsourceA/premulonce/ninewholeprivateframes verified. ActualFaceGen6706/6707 SW/CPU sixstates
logical128/mip2/alpha/Vflip/independentintegerFNV/fixed256atlas verified;
no completeGPUheadappearance/newbakeclaim. UI801 registration-only;allParticleInstancesTextureN
4094/4096/presentdefinitions audited, missingdefinitions/siblings explicitunknown.
Activewidget/particle/fullmodel appearance notcertified. Code unchanged25/currentbuiltEXE707a588b
byteexact to testedGame. Nativevalidation/realmenu11/0/11/fourprivateframes/normalquit:
expanded/new-groups-thirty-ninth-batch-check.json, expanded/new-groups-thirty-ninth-live-check.json
and expanded/new-groups-thirty-ninth-runtime-check.json. Five frozenreviews;
heldresults keptimmutable/no retries. OrdinaryGitPNG/RESbuiltwithGame.

Fortieth accepted packet: facecolor6786, UI2079, effect4089, equipment6213/7005/5374.
Six separate builtin generations. Pack2,451 textures/4,902 aliases/130 archives/7,819,875,172 bytes.
Prior2,445 source/PNG/native hashes unchanged. All6actualnativeRGBA equal privatewhole4x
predictions/fullsourceA/UV/material/count/nativepaintscale. Scalaronly/no RGBrepair/retries.
6786 keeps TWOverydim softbrowwashes/blackcorner;2079 redDOWNchevron/NPOT40x24/fullrectangularA;
4089 TWOoldsoftgraybrownpaintdomains. 6213 oldlowergraybar present in fullBEFOREmatrix,
promptT-description separatelycorrectedPOSTCALL;7005 twoUVfields/faintpinkpaint/junctionsoftening
disclosed;5374 two rows/fourcolorfields/fullhiddenRGB/sourceoldsteppedshade/topdarkbluecolumn
retained. Ordinary actualopaqueMaterial5374 meansA>16 is not renderingvisibility.
Individualcaveats:expanded/coordinator-art-review-fortieth.json. Rootheld6920 coarsecolorblocks,
5796 backgroundstoneinterpretation, separatefromfrozenworkerreviews/no retries.
Five individuallyconstantRGBA2198/3305/5101/5145/6803 originalretained/0calls;
source/release/actualMaterials/Templates/Models verified in
expanded/coordinator-original-only-equipment-thirty-eighth.json/no categoryexclusion.
ActualgenericGPU2079/4089/5374 ninewholeprivateframes on/off/on/whole/middle/reverse/clippedUV/
placement/density4vs1/fullsourceA verified;Transparentpremulonce2079/4089/Ordinary5374straightRGB.
Actual6786FaceGen46/THMID1 logical128SW/mip2/independentintegeralpha/Vflip/FNV/fixed256atlas
three states verified/no newbake/fullGPUheadappearanceclaim. Genericprobes do not certify
activewidget/particle/full3Dmaterialappearance. Source6SHA25unchanged/currentbuilt707EXEexact.
Nativevalidation/realmenu11/0/11/fourprivateframes/normalquit:
expanded/new-groups-fortieth-batch-check.json, expanded/new-groups-fortieth-live-check.json,
expanded/new-groups-fortieth-runtime-check.json. Fivefrozenreviews/ordinaryGitPNG/RESbuiltwithGame.

Forty-first accepted packet: effects4101/2575, UI3789/3886, equipment6668.
Five separate builtin generations; scalar brightness only. Pack2,456 textures/4,912 aliases/
130 archives/7,820,948,534 bytes. Prior2,451 source/PNG/native hashes unchanged.
All5whole actualnativeRGBA equal privately reviewed4x predictions/fullsourceA/UV/material/
count/nativepaintscale. Individual weakpalette/blur caveats disclosed in
expanded/coordinator-art-review-forty-first.json. No artisticRGBrepair/retries.
4101 THREEdimRGBdomains;2575 softwarmpaint/usefulRGB/A0/TransparentAdd;
3789/3886 oldgoldcircle/darkannulus/blackglyph;6668 two dimUVfields/faintredwash/matte.
Rootheld3045 steelgrain/directionalleather; sixworkerreviews frozen, rootdecision separate.
Original6780 retained:16383blackRGB+ONEwhiteopaque authoringcorner/variabletwo-browAlpha,
NOTconstantRGBA; exactsource/release/FaceGen40/THMID1/no builtin/noHDartentry.
GPU3789/2575/4101 ninewholeprivate on/off/on frames/whole/middle/reverse/clippedUV/
placement/density4vs1/fullA verified. Transparentpremulonce3789/4101;Add2575/noPremul.
Original6780 actualFaceGen128/mip0/all3sameoriginal source+CPUatlas hashes match independent
integeralpha/Vflip/FNV/fixed256atlas. DirectCTeamMarker UI524/530 consumers proven;
genericprobes do not certify activechaptermap/particles/fullGPUhead appearance.
Realmenu11/0/11/fourwholeprivateframes/normalquit0/ownedGameabsent verified.
Current performance limited:HD3.71FPS/original3.81FPS, delay chiefly in present;
prior60FPS not reproduced/cause unestablished/no performance acceptance claim.
SixsourceSHA25unchanged/currentbuilt707EXEexact. Three known promptLF entries proven,
oldmanifests frozen;Eq39 supplementalalias audit separately POSTCALLcorrected1818/noaliases,
correctBEFOREaudit/raw/ref/call/reviews unchanged. Oldconsumerprose clarification:
2079 UI399 Control1951only;1952 belongs2080/UI398. Proofs:
expanded/new-groups-forty-first-batch-check.json, expanded/new-groups-forty-first-live-check.json,
expanded/new-groups-forty-first-runtime-check.json. OrdinaryGitPNG/RESbuiltwithGame.

Forty-second accepted packet:UI2251/effects1764+5298,3separate builtin generations.
Pack2,459 textures/4,918 aliases/130archives/7,821,254,526bytes/prior2,456hashes exact.
WholeactualnativeRGBA3 equal private4x predictions/fullsourceA/UV/material/count/nativepaintscale;
2251oldgraycircle+cross,1764softcyan/5298softgreen glow; localpalette caveats in
expanded/coordinator-art-review-forty-second.json/scalaronly/noartistRGB/retries.
Rootheld6691 strongernearwhite originalreflection, notfalseanatomy/countclaim;
eightworkerreviews frozen/other12held saved. Original6692 retainedblackRGB16383+
ONEwhiteopaquecorner/fullgradedbeardAlpha/NOTconstantRGBA/FaceGen22THMID1/0calls/noHDartentry.
GPU2251/1764/5298 ninewholeprivateonoffonframes/whole+middle+reverse+clippedUV/
placement/density4vs1/fullA;Transparent2251premulonce(A255)/Add1764+5298A0usefulRGBnoPremul.
Directdisabledheal UI426/particleTextureN+definitions/siblingsmissingunknown audited.
Original6692 actualSW128/mip0/all3sameoriginal/source+CPUintegeralpha/Vflip/FNV/fixed256atlas.
Genericprobes do not certify activewidget/particles/full3Dhead appearance.
SixsourceSHA25unchanged/currentbuilt707EXEexact/realmenu11/0/11/pending0/normalquit/ownedGameabsent.
Current120frames:3.74FPS/p95278.12ms, no allmodelperformanceclaim;
prior41 slowpresent with bothHD/original qualification remains in itsreport.
Two known promptLF entries proven/oldmanifest frozen/two nestedclothmanifests fullyverified;
immutable before-call portablecopies numeric only. Proofs:
expanded/new-groups-forty-second-batch-check.json,expanded/new-groups-forty-second-live-check.json,
expanded/new-groups-forty-second-runtime-check.json. OrdinaryGitPNG/RESbuiltwithGame.

Forty-third accepted packet:UI3790/3419,2 separate builtin calls.
Pack2,461 textures/4,922 aliases/130archives/7,821,594,562bytes; prior2,459source/PNG/nativehashes exact.
WholeactualnativeRGBA2 equal private4x predictions/fullsourceA/UV/material/count/nativepaintscale/globalgain/defaultpad0.
3790oneoldambercircle/oldbrownperimeter/zero glyphs;3419threebrassforms/threesilvertips/oldrearstrip.
Localbrownperimeter/quietouterwash3790 and oldrearstripbrightness3419 caveats disclosed in expanded/coordinator-art-review-forty-third.json.
Rootheld4915 shiftedsoftcolorfalloff/5299 rougherlocalpaintcontrast;5299oldswirl already source/no false newring claim.
Fiveworkerreviews frozen/nineotherheld preserved/no RGBartrepair/retries.
GPU3790NPOT41/3419 sixwholeprivateonoffonframes/whole+middle+reverse+clippedUV/placement/density4vs1/fullA/premulonce.
ActualdirectpMovingUI525ChapterMap/ShortBurstUI500iUnitIconBar353/521 audited byreadonlyDB/source;
genericGPU does not certify activewidget/chaptermap/fullmodelappearance. No newFaceGen/original-only entries.
SixsourceSHA25 unchanged/currentbuilt707EXEexact/realmenu11/0/11/pending0/fourprivateframes/normalquit/ownedGameabsent.
Current120frames:3.73FPS/p95279.39ms; no performanceacceptanceclaim;
prior42slowpresent/prior41independent original+HDslowqualification retained/causeunestablished.
AllfrozenworkerSHA/customUIraw/call/BEFORE exact; actualargument+oneLF already canonical/no manifest or argument rewriting.
Proofs:expanded/new-groups-forty-third-batch-check.json,expanded/new-groups-forty-third-live-check.json,
expanded/new-groups-forty-third-runtime-check.json. OrdinaryGitPNG/RESbuiltwithGame.

Forty-fourth accepted packet:UI6453/effect4804,2 separate builtin calls.
Pack2,463 textures/4,926 aliases/130archives/7,823,189,374bytes; prior2,461source/PNG/nativehashes exact.
Two wholeactualnativeRGBA equal privately viewed4x predictions/fullsourceA/globalgain/defaultpadding.
WholeUV/material/oldparts/nativepaintscale reviewed;6453caps rounder/dimmer and oldgraybackground finer mottle disclosed.
4804native whiteclippedarea87→125(+44%,onepixelboundary expansion),halo centroid(+.492,−.278),
someweakouterbluebits quantizezero; no exactwhitearea/allweakpixel/topology identity claim.
Original-only7506 is analyticflatadditiveUI tint:inner44squareRGBA92/56/10/0 plusoldthincolorbands,
NOTentireconstantRGBA. ActualUI945/CComplexButtonFlash timedadditivetint/noAI/noHDartentry.
GPU6453/4804/7506onoffon9wholeprivateframes/whole+middle+reverse+clippedUV/placement/density4vs1.
6453Transparentpremulonce;4804/7506AddA0usefulRGB/noPremul.7506 original48/density1/mips1allstates;
diagnostic hd isglobalenable1/0/1,qualified independently. ActualdisabledToolUI850,
allTextureN18ParticleInstances4804+FinalElement51435.LightFlareTexture and UI945 source/DB proven;
genericGPU does not certify activewidget/particle/flare appearance. Fivefrozenworkerreviews/nineheld/no retries.
Knownprompt4804 exactactualargument+oneadditionalserviceLF proven/originalmanifest frozen;
raw/call/ref/BEFORE/workerreviews immutable. CustomUI51/52 allfiles verified.
Source6SHA25 unchanged/current707EXE exact/no newFaceGenbake. Menu11/0/11/pending0/fourprivateframes,
120frames3.74FPS/p95278.17ms/normalquit/ownedGameabsent.
Performanceacceptanceclaim:false;prior41independentoriginalandHDslow qualification retained/causeunestablished.
Proofs:expanded/new-groups-forty-fourth-{batch,live,runtime}-check.json.
OrdinaryGitPNG/RESbuiltwithGame;1,231verified originals remain.

Forty-fifth accepted packet:UI2249/effect5935,2 separate builtin calls.
Pack2,465textures/4,930aliases/130archives/7,824,784,186bytes; prior2,463source/PNG/nativehashes exact.
Two wholeactualnativeRGBA equal private4x predictions/fullsourceA/UV/material/count/nativepaintscale/globalgain/defaultpad0.
2249oldgraydiagonalshaft/head/fullgraybackground retained;rounder/darkerhead andbroaderoldpalewash disclosed.
5935oldsoftcyanbluehalo/flatwhiteinterior retained;5008oldwhitepixels mean248.49/248.11/248.04vs255,
centroid(−.308,−.468)nativepx/slightquantization disclosed;no exactwhiteplateauidentity/newrings/rays/surfaces.
Original-only6703constantblackartRGB+variableA and6783blackRGB16383+onewhiteopaqueauthorcorner/variableA/twobrows:
NOTconstantRGBA/fulloriginalRGBA retained/noAI/noHDartentries/no familyoralpha ban.
ActualFaceGen33/43THMID1 withallsource siblinglayers/unknownweights;6SWonoffon checks independentintegeralpha/Vflip/FNV,
original128/mip0/fixed256atlas;no newbake/fullGPUheadclaim.
GPU2249/5935sixwholeprivateonoffonframes/whole+middle+reverse+clippedUV/placement/density4vs1/pending0;
2249TransparentpremulonceA255/5935AddA0usefulRGBnoPremul. ActualUI424CreateButton384/424disabledgrenadeattack and
ParticleInstance4111allTextureN/sibling4737unknown audited;genericGPU notactivewidget/particleappearance.
Threeworkerreviews frozen/fourheld preserved;known5935prompt exactargument+oneadditionalserviceLF independentlyproven,
originalmanifest frozen/raw/call/ref/BEFORE/reviews immutable/customUI53 allfilesverified.
Source6SHA25/current707EXE exact;menu11/0/11/pending0/fourprivateframes/120frames3.73FPS/
p95278.43ms/normalquit/ownedGameabsent. Performanceacceptanceclaim:false;
prior41independentoriginalandHDslow qualified/causeunestablished.
Proofs:expanded/new-groups-forty-fifth-{batch,live,runtime}-check.json.
OrdinaryGitPNG/RESbuiltwithGame;1,227verified originals remain.

Forty-sixth accepted packet:UI4841/4842,2 separate builtin calls.
Pack2,467textures/4,934aliases/130archives/7,825,177,498bytes; prior2,465source/PNG/nativehashes exact.
Two wholeactualnativeRGBA equal private4x predictions/fullsourceA0..255/UV/material/count/nativepaintscale/scalar/defaultpad0.
Each oldJapanesedisk/softpaintfield/oldblackmargins/nativecontour kept;4841richerred/warmbeigecloth,
4842faintgraymottle slightlyclearer/colorcentroid caveats disclosed, no exactRGB/contouridentity claim.
Transparentpremulonce/GPU6wholeprivateonoffonframes/whole+middle+reverse+clippedUV/placement/density4vs1/pending0.
ActualNationalities6UI588/589/Sides1,3Nationality3/iCharGen550pNation3Set(normal,disabled) proven;
genericGPU notactiveCharacterScreen/flag3Dappearance. No original-only/newFaceGenbake/source6SHA25/current707EXE exact.
Sevenfrozenworkerreviews/eightotherworkerheld preserved;no rootwholeprivateview claim forallworkerheld.
Rootheld6812/6816 moreexplicitfiber/metalgrain/oldgrayrimcontrast,2523moreconcretecentralgroovecontrast/brighteroldflecks.
Twoovals/oldswirl/flecks already source, notnewphysicalparts;workerreadyreviews remainimmutable/rootdecisions separate.
No RGBartrepair/retries;raw/prompts/call/ref/BEFORE unchanged. Bothcanonicalprompts alreadyexactargument+oneadditionalLF,
no manifest rewriting;customUI54/55/clues2 allfilesverified. Menu11/0/11/pending0/fourprivateframes,
120frames59.83FPS/p9517.74ms/normalquit/ownedGameabsent.
CurrentHD120frame againnear60FPS; prior41independentoriginalandHDslow/42–45HDslow retained. Causeoftransitionunestablished; no source/driver/systemchanges byroot/no causalfix/generalperformanceacceptanceclaim.
Proofs:expanded/new-groups-forty-sixth-{batch,live,runtime}-check.json.
OrdinaryGitPNG/RESbuiltwithGame;1,225verified originals remain.

Forty-seventh accepted packet:UI4837/4838 andclothing7607/7610,4 separatebuiltin calls.
Pack2,471textures/4,942aliases/130archives/7,828,367,122bytes; prior2,467source/PNG/nativehashes exact.
Four wholeactualnativeRGBA equal private4x predictions/fullsourceA/scalar/defaultpad0.
UI oldTHREEItalianflagfields/softpaintwash/blackmargins kept, colorred/cream/green/gray andsmootherwash caveats disclosed.
Clothing oldred/graybluepaint/brownorTanplane/blackUV/ONEangulartrace/ONEpalerightcornerstroke/TWOdimpatches retained;
cleareroldgraywash/dimmertrace/broaderpalepeak/localphase caveats reviewed, no exactRGB/contouridentity claim.
FullUV/count/material/nativepaintscale/sourceblur retained, no newfiber/metal/physicalparts.
UITransparentpremulonce/fullA0..255;clothingOrdinaryA255straightRGB. ActualNationalities3/UI586/587/Sides1,3Nationality1/iCharGen540
andMaterials5779/5776->Templates->Models proven. GPU12wholeprivateonoffon/full+middle+reverse+clippedUV/placement/density4vs1/pending0;
notactiveCharacterScreen/3Dclothingappearance. No original-only/newFaceGenbake/source6SHA25/current707EXE exact.
3095concreteworker recommendation separatelypreserved/pendingindependentrootclassification/no queueoriginalentry.
Sixfrozenworkerreviews/sevenworkerholds preserved/no rootwholeallheldviewclaim. Rootheld6813/14/17moreconcretefinegrain/lowerframepaint/oldovalrimcontrast+weakboundaryphase,
2373brighterthinzigzag/weakjunctiontrace loss; oldovals/branches already source, notnewphysicalpartcounts.
Workerreadyreviews immutable/rootdecisions separate/noRGBartrepair/retries/raw-call-ref-BEFORE exact.
ONLYtwo knownacceptedCl44promptLF entries7607/7610 independentlyproven/namedproof/oldmanifest frozen;
no oldchildfreeze/blindrefresh. CustomUI56/finalother2 allfilesverified. Menu11/0/11/pending0/fourprivateframes,
120frames59.97FPS/p9516.78ms/normalquit/ownedGameabsent. Prior41independentoriginal+HDslow/42–45HDslow
and46near60FPS retained/causeoftransitionunestablished/no source-driver-systemchanges byroot/no causalfix/generalperformanceacceptanceclaim.
Proofs:expanded/new-groups-forty-seventh-{batch,live,runtime}-check.json. OrdinaryGitPNG/RESbuiltwithGame;1,221originalsremain.

Forty-eighth accepted packet:weapon1931/1942 andcharacterbitmap799,3 separatebuiltin calls.
Pack2,474textures/4,948aliases/130archives/7,832,561,582bytes; prior2,471source/PNG/nativehashes exact.
Three actualnativeRGBA equal private4x predictions, OrdinaryfullsourceA0..255forweapons/A255for799/straightRGB/scalar/defaultpad0.
Weapons entire4:1source->wholeNEAREST3:1guide->ENTIRErawinverse4:1, no crop/pad/fit/register/artistRGB.
Oldbrown-graypaint/SEVEN1931upperstrokes/TWOlowerpalepanels/THREE1942woodlightmarks retained;
warmerpalette/cleareroldoutlines/dimmerlowerbrown/localphase caveats disclosed.799 authoredUPSIDE-DOWN UV,
ONEear/TWOeyeshadows/nose/closedmouth/hairwash/TWOtopbandfields kept; oldskin-hairpaint clearer/localwarmer-brighter,
no exactRGBidentity/newteeth/eyes/photofibers. FullUV/count/material/nativepaintscale/sourceblur keptwithdisclosedlimits.
Threeindividualoriginals retained:no AI/noHDartentries.3095actualObjects1228Model1->CM1250Effect493->PI1515Particle194Texture0:
independent3264blackRGB/A0..16/sprite0 tracks+actualshader zeroRGB/attenuatingcoverageA provepurpose; hardcodedMapBorder1836/CM2083Particle0 unrelated.
2482quietblue147x17functionalgradient/33Type15controls/UI470/CProgressBarcrop-scale byfValue/notconstantRGBA oridenticalrows.
4592entire16squareRGBA2/203/0/0/Material2860Template1840Models/uniformtint/usefulAddRGB-A0/noPremul/blackening.
Wholehistory-releaseRGBA/typedpurpose/rootwholeA+RGB/GPU9exactoriginaldensity1; no alpha/type/name/category blanketban.
ArtGPU9onoffon/whole-middle-reverse-clippedUV/placement/density4vs1/fullA/pending0, originalGPU9density1allstates withGLOBALhd1/0/1.
GenericGPU notactiveweapon/Head/particle/widget appearance; no newFaceGenbake/source6SHA25/current707EXE exact.
Eightfrozenworkerreviews/eightworkerholds preserved/no rootwholeallheldviewclaim; rootheld4502finepaintgrain/relief-likeoldedges/localstrength,
notnewhardware/macrocounts. Raw-call-ref-BEFORE immutable/no repeats/RGBrepair. KnownLF0/all3alreadycanonical exactarg+ONELF,
no oldmanifest/childfreeze refresh; CustomUI57/58/59 allfilesverified. Menu11/0/11/pending0/fourprivatewholeframes/120HDframes
59.92FPS/p9518.07ms/normalquit/ownedGameabsent. Prior41independentoriginal+HDslow/42–45slow and46/47near60 retained;
causeoftransitionunestablished/no source-driver-systemchangesbyroot/no causalfix/generalperformanceacceptanceclaim.
Proofs:expanded/new-groups-forty-eighth-{batch,live,runtime}-check.json andcoordinator-original-only-forty-eighth.json.
OrdinaryGitPNG/RESbuiltwithGame;1,215verifiedoriginalsremain.

Forty-ninth accepted packet:1932/1939/1940/1951 weaponUV,4729 darkplatformUV and6543grayicon,
six separatebuiltin calls. Pack2,480textures/4,960aliases/130archives/7,850,737,210bytes;
prior2,474sourceobjects/PNG/nativehashes immutable; sixactualnativeRGBA equalprivate4xpredictions.
OrdinaryfullsourceA0..255weapons/A255UI/straightRGB/scalar/defaultpad0. Fourwhole4:1weapons->NEAREST3:1guide
exactinverseRGB->ENTIRErawinverse4:1/no crop/pad/fit/register/artistRGB. Oldbrown-graypaint/weakmarks/nativecounts
keptmaterial/sourceblur/paintscale withcleareroldoutlines/localpalette-lightstrength/weakphase caveats.
1932two dimopaque pixels becameblack;1951two dimpixels lostRGB; wholeartreview notautomaticthresholdacceptance.
1939THREEoldstrokes/1940olddarklines+THREElightmarks clearer, brightnessmaxima notgeometry/components.
No newhardware/glyphs/photo fibers/abrasive machining.4729ONEslanted oldgrain-paintedpanel/topstrip/leftring/lowergrayfields
keptmattematerial; existinggrain/frameedgesmoredefined/localmottlechanging.6543FOURnotchedarms/EIGHTtips/FOURdiagonalbars/
oldtriangularlines/lowerdarkwash retained, cleareroldedges/softnotchphase disclosed. WholeoriginalRGB/NN/A/raw/pure/native
privatelyviewed; GPU18wholeonoffon/full,middle,reverse,clippedUV/placement/density4vs1/pending0.
FreshactualDBMaterials/Templates/Models proveconsumers; genericGPU notactiveweapon/MainMenu/modelappearance.
No newFaceGenbake/source6SHA25/current707EXE+fixtureexact. Twelvefrozenreviews/eightworkerholds immutable,
no rootwholeallheldviewclaim. Source-onlypurpose recommendations andunavailable4620/7189 originalfallback individuallyrootpending;
no originaldispositions thisportion/no blanketalpha-type-name-familyban. Bothactualerrors savedwithhonestcapturelimitations,
no raw/retry/bypass. Allsix alreadycanonical actualarg+ONE LF/knownLF0/raw-call-ref-BEFORE-SHA-frozen unchanged.
CustomUI60–64/sevenflatmanifests verified; oversizedJSON losslessportable-large-proof-forty-ninth.json/
verify_portable_large_forty_ninth.py --restore forabsentoriginalsonly. PNGordinaryGit/RESbuiltwithGame.
Menu11/0/11/pending0/fourwholeprivateframes/120HDframes:59.96FPS/p9516.84ms/
normalquit/ownGameabsent. Prior41independentoriginal+HDslow/42–45slow and46–48near60 retained/causeunestablished/
no source-driver-systemchangesbyroot/no causalfix/generalperformanceacceptanceclaim.
Proofs:expanded/new-groups-forty-ninth-{batch,live,runtime}-check.json;1,209verifiedoriginalsremain.

Fiftieth accepted packet: equipment5407 and normal/disabled class icons7448/7450/7449/7451.
Five separate builtin calls, fullUV/sourceA/scalar brightness; pack2,485textures/4,970aliases/
130archives/7,851,873,410bytes, prior2,480source/PNG/native hashes unchanged.
5407whole1:4->NEAREST1:3guide exactinverseRGB->ENTIRErawinverse1:4; THREEprojections/FIVEtips/
ONEoval/leftstem retained, olddarkpigment notnewhole. OrdinarystraightRGB; fourTransparent
sourceA/nativepremulONCE, sourceorientation/oldloops/U anddimdisabledroles retained with
cleareroldedges/localsoftphase/washstrength caveats. No newhardware/photosurface.
Sevenfrozen workerreviews/fiveworkerheld/no rootwholeallheldview claim. Rootheld1930 separate
from immutableworkerready: oldsoftbrownwash becomesdirectionallines/lowergrayweakcadence changes.
No retry/RGBrepair; individualoriginalpurpose recommendations pending rootoriginalGPU.
FreshtypedDBMaterials/Templates/Models/RPGClasses/RPGPers/DataRPG/iCharGen564 consumers,
genericGPU notactualactiveequipmentmodel/CharacterCreationwidget. All15wholeprivateGPUframes
andfourmenu frames reviewed/full-middle-reverse-clippedUV/density4vs1/pending0/menu11/0/11.
FiveactualnativeRGBA equalpredictions/allpackvalidated/source6SHA25/current707EXE/no newFaceGenbake.
All5alreadyactualarg+ONE LF/knownLF0/oldraw-call-ref-BEFORE-flat-custom exact.
PNGordinaryGit/RESbuiltwithGame/120HDframes60.00FPS/p9516.97ms/normalquit0/ownGameabsent.
Historical41independentoriginal+HDslow/42–45slow/46–49near60 retained/causeunestablished/
no rootcode-driver-systemchange/no causalfix/generalperformanceacceptanceclaim.
Proofs: expanded/new-groups-fiftieth-{batch,live,runtime}-check.json;1,204 verified originals remain.

Fifty-first accepted packet: equipment4948/6669 and disabled UI5021/5034.
Four separate builtin calls/fullUV/sourceA/scalar brightness;2,489textures/4,978aliases/
130archives/7,852,965,882bytes; prior2,485source/PNG/native hashes unchanged.
Original painted material/scale/count/orientation retained with clearer oldedges/local
softphase/wash-strength caveats;6669old clipped redcream mark notcompleted into a badge.
TwoOrdinary straightRGB/twoTransparent fullsourceA/premulONCE; actualnativeRGBA exact.
Seven frozen workerreviews; rootheld3784/4827/4828 materialgrain/weakfaceidentity changes,
immutableworkerready unchanged; fiveotherworkerheld/no rootwholeallheld-review claim.
Individualoriginals4594 usefulgreenA0 and1133 solidR156vs155 preserved;4620/7189 single
actual builtin service refusals/no raw/no retry/bypass. Bothnative aliases preserved.
4518/1885/1893/4564 compressedzero vsuncompressed whiteUVwireframe/A0 deferred,
notclassified empty fromcompressed-only proof. Freshbothalias history/release audit;
DBFormat/bIsDXT andactual gfx_texture_usedxt establish variantselector,4948RGBcodec differs/A255equal.
TypedMaterials/Templates/Models/UI/iStorePanel367/371 consumers proven; genericGPU
notactual active3Dmodel/widgetappearance. All12artGPU/24originalGPU/fourmenu frames
privatelyviewed/full-middle-reverse-clippedUV/density4vs1-or-original1/pending0/menu11/0/11.
Fullpackvalidation/source6SHA25/current707EXE/no newFaceGenbake; all4actualarg+ONE LF/
knownLF0/oldraw-call-ref-BEFORE-manifests exact. PNGordinaryGit/RESbuiltwithGame.
120HDframes59.96FPS/p9518.38ms/normalquit0/ownGameabsent.
Historical41–45slow/46–50near60 causeunestablished/no causalfix/generalperformanceacceptanceclaim.
Proofs: expanded/new-groups-fifty-first-{batch,live,runtime,original-runtime}-check.json;
1,196 verified originals remain.

Fifty-second accepted packet: four disabled UI5038/5345/4633/7228, fourseparatebuiltin
calls/fullUV/sourceA/scalar/TransparentpremulONCE.2,493textures/4,986aliases/
130archives/7,853,752,506bytes; prior2,489source/PNG/native hashes unchanged.
Softmattepaint/scale/count/storedorientation retained witholdedgeclarity/localwash/
rounding caveats.5038dot/5345junctionpatch/4633lowerwash alreadySOURCE, notnewstud/lens.
7228storedRIGHT despiteUI936LeftD; actualnativeRGBA allfour exactpredictions.
Five frozenreviews/fiveworkerholds/no rootwholeallheldview claim.6399/6400 individual
original-purpose recommendations pendingroot/no dispositions. FreshtypedDBUI and
iStorePanel/iUnitIconBar/iCustomGame/CComplexButton consumers. AllDB8888 primaryselector
despiteconfigusedxt0; alternateabsence3/7228alternateaudited/notselected, freshrootPOSTCALL
audit distinct fromimmutableBEFORE/no falsefullaliasRGBAidentity.
All12wholeprivateartGPU/fourmenu frames/full-middle-reverse-clippedUV/density4vs1/
pending0/menu11/0/11; genericGPU notactualactivewidgetappearance. Fullpackvalidation/
source6SHA25/current707EXE/no newFaceGenbake;4actualarg+ONE LF/knownLF0/oldraw-call-ref-BEFORE-manifests exact.
PNGordinaryGit/RESbuiltwithGame/120HDframes59.84FPS/p9518.15ms/
normalquit0/ownGameabsent. Historical41–45slow/46–51near60 causeunestablished/no causalfix/
generalperformanceacceptanceclaim. Proofs:expanded/new-groups-fifty-second-{batch,live,runtime}-check.json.
1,192 verified originals remain.

All 14,340 original, raw AI-generated, normalized, calibrated, helper and rejected PNGs
are ordinary Git files (11,591,796,929 bytes before Git deduplication/compression).
No ZIP, LFS or release download is required after cloning. Large asset additions
are committed and pushed in bounded batches to stay below GitHub's push limit.
The initial import used 17 data pushes. Every remote PNG blob ID and size matched
the independently verified source bytes, and no ZIP or generated native files
were included (`expanded/repository-transfer-check.json`).

Windows and Linux Game builds include the `S2HDTextures` dependency by default.
It converts the accepted PNGs into `res-hd` next to the game executable, including
numbered native shards and a generated manifest. Python 3.11+ and Pillow are
required; point CMake's `Python3_EXECUTABLE` at the appropriate interpreter.
The game discovers that adjacent HD directory even with the original game's
install directory as its working directory. Existing working-directory HD packs
remain a fallback. Original/Steam resources are not modified.

The builder's `--incremental` mode checks referenced inputs and every output's
size and modification time; unchanged builds skip conversion, while a changed
PNG, missing shard or modified output triggers regeneration. `build-state.json`
is generated only after a successful build. `-DS2_BUILD_HD_TEXTURES=OFF` omits
the build step; this build option is separate from the runtime HD toggle.
Generated `res-hd` files remain outside Git. No AI service or runtime upscaling
is involved in building the native pack.

For manual brightness matching and rebuilding, use Python, Pillow and NumPy:

```sh
python assets/terrain-hd/match_brightness.py
python assets/terrain-hd/build_pack.py
python assets/terrain-hd/validate_pack.py
```

Brightness matching always starts from the approved uncalibrated PNGs, so repeated
runs do not accumulate darkening. `*-balanced.png` are the calibrated inputs;
`brightness-report.json` records the measured before/after/reference luminance
and gain for every cell. Transparent sprite measurement uses premultiplied RGB
as stored in the native pack; opaque maps use their original RGB encoding.
Brightness correction changes RGB values while dimensions and generated alpha
remain identical. On transparent sprites, RGB edge padding fills otherwise empty
generated color texels beneath the restored source silhouette, using the nearest
existing color within the same atlas cell. It prevents black holes when generated
fine stems shift slightly, without changing the native silhouette. Padding counts
are recorded per cell. Invisible generated RGB beneath both an empty source mask
and near-zero generated alpha is cleared before correction. This prevents stray
red/yellow hidden pixels from leaking into ordinary-texture mipmaps; visible
foreground and PNG alpha are preserved, including legacy all-zero-alpha RGB
materials. `rgb_hidden_clear_texels` records the cleanup per cell.
Reviewed extreme-aspect strips or isolated scraps may use RGB or
alpha bounding-box registration to remove generator letterboxing and restore the
source occupied UV region; the original output and registration are preserved.
When ordinary 8-bit rounding cannot match a cell's brightness closely enough,
an 8×8 ordered rounding pattern distributes the sub-integer RGB correction.
It changes quantization by at most one channel level and preserves alpha.

The six book-kiosk textures (3795, 3796, 3797, 3953, 6206 and 6207) contain newly generated
period detective, adventure, travel and science cover illustrations, as requested
by the user. These are content replacements, rather than faithful enlargements
of the original cover artwork. Damaged variants share the intact kiosk's cover
collection; wood-only references and binary void guides preserve their damage.
The second stand also shares its new covers between intact and damaged versions.
`content_replacement` in `sources.json` records this exception. All six receive
the same numerical brightness matching as the other world assets.

The deterministic builder writes `res-hd/Textures.res`, numbered shards and
`manifest.json`, with provenance, hashes, actual raster sizes and mip counts.
Native BGRA MMP strips contain nine mip levels down to 4×1; square ground maps
contain twelve and the sprite atlas eleven, down to 1×1. Both normal and
non-DXT aliases point to the same payload. Generation and conversion happens
offline; the game loads ready native resources.

Copy `res-hd` beside the existing `res` directory. The engine mounts it at
startup and after mod activation. Priority: active mods, HD pack, base resources.
Within each layer loose files override its archive; missing images fall through.
The **HD textures** graphics option defaults to enabled and is saved as
`gfx_hd_textures`. Turning it off skips the entire HD layer and invalidates both
software and GPU source caches, so terrain is rebuilt at original sizes when
the current scene resumes. Turning it back on reloads HD without restarting.
Database
IDs, saves and world sizes are unchanged. The HD layer accepts texture categories
only and is excluded from the network compatibility fingerprint.

Terrain color patches with denser inputs grow to at most 1024×1024 and own their
buffers. Original bump maps and masks retain their resolution and normalized
mapping, including spots whose diffuse alpha image is larger. Other patches stay
at their original resolution. Color patches retain mipmaps down to 16×16.
The transparent sprite cache uses 4096×4096 with HD enabled and the original
1024×1024 with HD disabled. Crowns, grass sheets and other effects must fit
together because the renderer batches them into one texture. The setting
recreates the cache before new sprite allocations; source nodes are invalidated
by the same resource revision. World geometry, atlas order and UI are retained.

The builder splits large packs into `Textures.res`, `Textures-0001.res`, etc.
The loader discovers these numbered HD archives automatically and ignores every
one while HD is off. Each ordinary archive is capped at 64 MiB. One oversized
texture stays intact in its own archive. Previous shards are removed only if
they are listed in the previous build manifest and their content is unchanged.

Expanded assets and per-asset generation prompts are under `expanded/`.
`expanded/queue.json` records full requested scope, original/release identity,
processing status and exclusions. Transparent ground decals use the historical
alpha mask resampled at HD density when encoding native resources. The generated
PNG alpha remains available unchanged for provenance. Brightness measurements
use the final native mask so dark translucent edges remain correctly calibrated.

## Verification

The second new-group package adds 29 textures and passed another Windows x64
Game build and full native validation: 2,011 textures, 4,022 aliases, 111 archives
and 6,706,321,654 bytes. All 1,982 previously accepted source objects, PNGs and
native MMP hashes stayed unchanged. Native luminance errors for the new 29 are
below 0.023 levels/255. `expanded/new-groups-second-batch-check.json` records
source/release identity, original alpha, dimensions, per-ID evidence and all
1,753 remaining verified source IDs. It covers eight clothing, seven ordinary
RGB565 equipment, eight particle sprites, four camp atlases and two training
targets. Reviewed side letterboxing in 6616/6618 is registered to exact original
occupied bounds. Sprite masks, particle pivots/timing/blend and atlas detail
counts remain original. The 6265 gradient retains its original native RGB and
structural alpha (0–251) through fallback.

`expanded/new-groups-second-live-check.json` records the final 2,011-texture
pack's isolated Windows/D3D11 menu check at 2560×1440. Terrain HD patches changed
11→0→11. The 120-frame capture averaged 59.97 FPS; the test process quit normally
with code 0. Captures were privately inspected; this validates switching in the
headquarters scene, not every model and particle in all missions.

The first new-group package passed a Windows x64 Game build and full native
validation on 2026-10-07: 1,982 textures, 3,964 aliases, 110 archives and
6,661,406,064 bytes. Every previously accepted 1,962 source object, PNG and native
MMP payload retained its hash. `expanded/new-groups-first-batch-check.json`
records all 20 IDs, source/release parity, dimensions, hashes and brightness errors.
Clothing/equipment/weapons retain full-canvas UVs. Only individually reviewed
single sprites 6091/581/582 use alpha-bbox registration. Both heads retain the
explicit technical near-black gum margin (0,16,64,34) in source logical pixels
before numerical gain; face and visible tooth artwork remain generated.
`test_source_rgb_regions.py` verifies bounded RGB restoration and exact native
unpremultiplication without changing alpha or pixels outside the reviewed region.

`expanded/new-groups-live-check.json` records the isolated Windows/D3D11 check
of the 1,982-texture pack at 2560×1440. The real graphics menu toggled HD off/on,
terrain patches changed 11→0→11 and 120 frames averaged 60.11 FPS. All captures
were privately inspected; the agent's test process quit normally with code 0.
This checks switching in that scene, not every new model or particle.

The RGB565 extension passed a Windows x64 Game build and full native validation
on 2026-10-07: 1,962 textures, 3,924 aliases, 110 archives and 6,627,588,404 bytes.
Every previous PNG and native texture payload retained its hash. The new assets
were privately inspected after per-island UV registration and numerical brightness
matching. `expanded/rgb565-batch-check.json` records commands, hashes and per-ID
dimensions/luminance. This extends the native checks; the earlier live game report
below still describes its original 1,952-texture scene.

`validate_pack.py` checks every archive hash, both aliases of every resource,
native dimensions, full mip payload, original alpha and brightness against the
historical source (within 0.1 luminance levels). It reads individual payloads
instead of keeping the entire multi-gigabyte pack in memory. The reproducible
result is saved in `res-hd/validation.json`; `expanded/queue.json` is marked
validated only after the complete check passes. The detailed coverage report is
`expanded/coverage.json`.

The CMake integration passed full Game builds on Windows and Linux. Both produced
the previously validated native manifest and the same 110 archive hashes beside
the executable. Full native validation also passed on the Windows build output.
Incremental tests cover unchanged builds, changed PNGs and alpha references,
manifest edits, missing shards and modified outputs. Resource opener regressions
on both platforms cover adjacent HD packs, working-directory fallback, HD opt-out
and exclusion from network resource directories. Results are recorded in
`expanded/source-build-check.json` and `expanded/incremental-build-check.json`.

The preceding 1,952-texture pack passed all native checks on 2026-10-07:
3,904 aliases in 110 archives, totalling 6,615,704,000 bytes. An isolated Windows
D3D11 game run at 2560×1440 then toggled HD off and back on through the actual
graphics menu. Visible HD terrain patches changed 11 → 0 → 11; the captured
original and restored HD frames and menu were privately inspected. Tracked GPU
texture memory was approximately 474 / 427 / 572 MiB across those states.
The final 120-frame capture averaged 59.97 FPS with v-sync, with a 16.84 ms
95th-percentile frame time. The game exited normally with code 0 and retained
HD enabled in its settings. `expanded/live-check.json` records the game hash,
measurements and evidence hashes. This verifies that scene and menu switching;
it is not a visual inspection of every building model in every mission.

`BgfxRendererTests --terrain-hd <absolute path to res-hd>` checks all five assets,
both resource aliases, dimensions, mip chains, sprite alpha encoding, four tile
rotations, mixed densities, color/bump spot masks and transparent-cache coexistence.
These checks and full Game builds passed on Windows/D3D11 and Linux x64/Vulkan.
The resource opener's earlier tests cover loose/packed priority and sync/async
reads. `S2_TERRAIN_AUDIT=1` records submitted source IDs in `_terrain_sources.log`.

A Windows headquarters save was captured at 2560×1440 at camera distances 25,
10 and 60, with originals and HD using the same settings. Unobstructed grass crops
initially differed by about 14–16 RGB levels in mean absolute difference, versus roughly
one level with the previous base-only prototype. This establishes a visible
change; image quality remains an artistic judgement. At distance 25, eleven HD
patches were visible and 55 cached. Tracked GPU textures totalled about 377 MiB.
After fixing the transparent cache, full-file load count was 127 rather than
over 26000 and frame rate was approximately 60 FPS with v-sync enabled.
All test games exited normally. Frames and comparison page are preserved outside
the repository under `lab/evidence/terrain-visible-hd-20261006`.

The calibrated pack was then checked on the same Windows scene and by the
Windows/Linux native renderer regressions. Texture average brightness matches
each historical atlas cell within 8-bit quantization. In two unobstructed grass
crops, the previous HD version was about 16% / 24% brighter than the original;
after calibration the crops are within approximately 1% / 5% of the original.
The test retained about 60 FPS with v-sync. Alpha and texture layout are unchanged.
The three-version comparison is under `lab/evidence/terrain-visible-hd-20261006/brightness`.


## Whole heads/hair category completed, 9 October 2026

All186 source IDs are accounted for:152 new HD,24 prior accepted immutable,
10 individually qualified functional originals. Eligible artwork remaining0.
All152 actual nativeRGBA equal the worker-reviewed forecasts; every actual
source alpha byte, both aliases and all mip bytes pass. The complete Game pack
has2,649 textures/5,298 aliases/140 archives/8,470,323,902 bytes. Prior2,497
source objects, PNGs and native payload hashes are unchanged. Actual on/off/on
menu, all41 new dynamic layers and nine FaceGen bakes pass; the own Game
exited normally. The category agent inspected all16 actual frames; the
coordinator did no artwork review. See expanded/new-groups-heads-complete-20261008-batch-check.json
and heads-complete-20261008/native-runtime-visual-review-final.json. Full
original frozen manifests also name local-only QA images; the separate
publication manifest explicitly excludes those private montages/previews.
World and heads/hair are complete. Clothing168 new artworks are worker-ready
and await the next single whole-category build/publication. Other categories
remain paused.

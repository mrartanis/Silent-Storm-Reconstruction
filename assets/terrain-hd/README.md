# HD textures

For the Russian build/install and authoring instructions, see
[HD-TEXTURES.md](../../HD-TEXTURES.md). To continue generation in a new Codex
session, use [CONTINUE-HD-TEXTURES.md](CONTINUE-HD-TEXTURES.md).

The offline HD pack expands the original five-asset landscape prototype to the
requested terrain, trees, buildings and environment. `expanded/queue.json`
records exact per-resource coverage and processing status; `res-hd/manifest.json`
records what is actually installed. Pending entries are not advertised as HD.
The completed pack contains **2,151 HD textures**: 178 terrain assets, 30 tree
assets, 1,754 building/environment assets, 48 clothing, 37 equipment, 59 weapons,
2 heads, 20 interface icons, 17 effects, 4 camp atlases and 2 training targets.
All 2,349 world-scope resources have a recorded disposition. The shared queue
contains 2,556 records, including 189 accepted new-group textures, the original
6265 structural gradient and 15 newly recorded missing historical sources.
Original fallback covers 257 technical maps,
74 structural/solid masks, 63 unavailable historical sources, one release mismatch
and ten resource IDs whose original-preserving image generation was unavailable
(nine distinct source images). No entries in that queue remain pending.
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
validation. There remain 1,611 verified originals outside the shared queue, including next
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

All 11,011 original, raw AI-generated, normalized, calibrated, helper and rejected PNGs
are ordinary Git files (9,918,598,883 bytes before Git deduplication/compression).
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

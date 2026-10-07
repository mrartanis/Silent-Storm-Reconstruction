# HD world textures

For the Russian build/install and authoring instructions, see
[HD-TEXTURES.md](../../HD-TEXTURES.md). To continue generation in a new Codex
session, use [CONTINUE-HD-TEXTURES.md](CONTINUE-HD-TEXTURES.md).

The offline HD pack expands the original five-asset landscape prototype to the
requested terrain, trees, buildings and environment. `expanded/queue.json`
records exact per-resource coverage and processing status; `res-hd/manifest.json`
records what is actually installed. Pending entries are not advertised as HD.
The completed pack contains **1,952 HD textures**: 178 terrain assets, 29 tree
assets and 1,745 building/environment assets. All 2,339 resources in the requested
scope have a recorded disposition. Original fallback covers 257 technical maps,
73 structural/solid masks, 48 unavailable historical sources, one release mismatch
and eight resource IDs whose original-preserving image generation was unavailable
(seven distinct source images). No entries in that queue remain pending.
An additional read-only audit found ten ordinary color RGB565 resources excluded
by the old format filter: 5076, 5168, 5169, 6997 and 7581–7586. They are genuine
bark, clock and medical-bed artwork, and all ten exactly match release pixels.
They remain a follow-up outside the completed 2,339-entry queue, documented in
`expanded/rgb565-followup.json`. Character, equipment, weapon, face, interface
and effect groups also require separate inventories. The full 5,801-texture DB
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

## Build and install

All 8,469 original, raw AI-generated, normalized, calibrated and rejected PNGs
are ordinary Git files (8,934,956,122 bytes before Git deduplication/compression).
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

The completed 1,952-texture pack passed all native checks on 2026-10-07:
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

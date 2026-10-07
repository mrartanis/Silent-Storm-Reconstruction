# Visible grass layers

All edit targets were decoded from historical Complete/Textures and matched
against the release resource by exact RGBA bytes. Each request used its own
original PNG as the only reference. The sprite request enabled true transparency.
Generated outputs are retained without alteration as *-generated.png.
Native map sizes require powers of two: outputs are resampled offline using
Lanczos to 2048 square (ground) or 1024 square (sprite sheet). This is packaging;
the generated raster's actual resolution is recorded in sources.json.

## 3242 — grass ground spot

Edit this exact 512x512 video-game ground albedo texture into a faithful high-definition 2048x2048 version (4x detail). Same top-down flat orthographic seamless grass surface, same placement and scale of large patches, same extremely dark olive green and muted yellow-brown palette, overall brightness near original (mean RGB about 33,34,10). Resolve individual fine narrow grass blades, dried strands and tiny soil detail, replacing pixel stair steps with convincing organic microstructure. Preserve the appearance and distribution of all large-scale patches; do not enlarge grass clumps or introduce flowers, stones, objects, thick tufts, directional lighting, shadows, highlights, depth-of-field or perspective. Edge-to-edge uniform material texture, tileable edges, no border, no text. This is an opaque diffuse map; no transparency. The increased texture detail must be clear at close magnification without changing the world's style or color.

## 6286 — grass1 ground spot

Faithfully upscale the attached 512x512 terrain grass texture to 2048x2048 (4x), reconstructing crisp organic microdetail. It is a flat top-down opaque diffuse ground map of mixed very dark olive grass, straw and earth. Preserve the existing macro-pattern, fine tangled short blade density, patch boundaries, orientation, muted yellow-brown/olive palette and original dark exposure (mean RGB about 35,34,11). Add believable fine distinct individual blades, dry fibers and soil microstructure at finer resolution, avoiding smooth paint smears and pixel edges. Do not change the material to bright lush grass, do not add objects or stones, flowers, thick isolated tufts, shadows, directional highlights, perspective, depth of field, borders or text. Edge-to-edge seamlessly tileable texture, no transparency. Same ground material and world scale as the input; sharp high-definition detail.

## 1906 — grass sprite atlas

Upscale and refine this exact grass sprite atlas from 256x256 to 1024x1024 (4x detail), with a real transparent background. The input is precisely 16 sparse isolated small grass blade clusters in a regular 4-column by 4-row grid, one sprite per cell. Preserve all sixteen sprites, their exact respective cells, orientations, positions, overall silhouettes, centers and thin narrow leaf scale. Each original 64x64 cell becomes 256x256. Keep generous completely transparent empty space around the blades and between cells, no grid lines, no border, no text, no ground, no shadows, no backing color and no extra clusters. Grass color stays very dark muted olive green with subdued yellow-green blade edges, matching the original. Replace stair-stepped old pixels with clear slender natural leaf contours and subtle central vein detail. Keep every blade's curved direction and fork structure recognizable, never expand into dense bushy tufts. Sprite silhouettes must not spill across cell boundaries. Transparent RGBA cutout sprites with clean semitransparent antialiased edges, not a black background.

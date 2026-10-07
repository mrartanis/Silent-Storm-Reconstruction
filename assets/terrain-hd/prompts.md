# Generation prompts

Built-in `image_gen`, two independent edits. Image 1 in each call is the edit
target: the original four 64×64 variants packed row-major into a 128×128 sheet.
Image 2 is the approved previous `*-approved.png` detail/style reference for the
first variant. Generated images were 1254×1254; the build assets are downsampled
and repacked offline, without modifying colors or painting pixels.

## Grass

Use case: detail-enhance. Asset type: Silent Storm terrain diffuse texture atlas, faithful AI super-resolution. Image 1 is the EDIT TARGET: a square 2 by 2 packing of four distinct 64x64 grass variants, in strict row-major order. Image 2 is the approved detail style reference for the first variant only. Upscale image 1 faithfully to a square high-resolution atlas, preferably 1024x1024 or larger. Preserve the exact positions, shapes, orientation, relative sizes, muted dark olive palette and large-scale features of EACH of the four original variants; keep all four distinct. Reconstruct finer strands and small soil details like the approved reference, do not invent larger plants or change the pattern scale. The image must be flat diffuse/albedo, orthographic, edge to edge, no margins or divider lines. EXACT FOUR equal square regions in a 2x2 grid, each independent texture variant. Preserve texture boundaries and make each individual region repeat cleanly. No lighting redesign, no highlights, no camera, no text, no frame, no additional objects. Avoid brightening, saturation, grid seams, large clumps or painterly smoothing.

## Soil

Use case: detail-enhance. Asset type: Silent Storm terrain diffuse texture atlas, faithful AI super-resolution. Image 1 is the EDIT TARGET: a square 2 by 2 packing of four distinct 64x64 ground and gravel variants, in strict row-major order. Image 2 is the approved detail style reference for the first variant only. Upscale image 1 faithfully to a square high-resolution atlas, preferably 1024x1024 or larger. Preserve the exact positions, shapes, orientation, relative sizes, subdued dark brown-gray palette and large-scale features of EACH of the four original variants; keep all four distinct. Reconstruct fine grit, tiny pebbles and soil texture like the approved reference, do not invent larger stones or change the material scale. Keep the source average brightness. The image must be flat diffuse/albedo, orthographic, edge to edge, no margins or divider lines. EXACT FOUR equal square regions in a 2x2 grid, each independent texture variant. Preserve texture boundaries and make each individual region repeat cleanly. No lighting redesign, no highlights, no camera, no text, no frame, no additional objects. Avoid brightening, saturation, grid seams, large rocks or painterly smoothing.

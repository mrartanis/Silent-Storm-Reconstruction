"""Reproduce the RGB565 A package helper inputs and registered private previews.
Run from any directory: python assets/terrain-hd/register_rgb565_a.py
Does not mutate shared metadata, imported outputs, brightness, or native packs.
"""
import json
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parent
EXPANDED = ROOT / "expanded"
BUILD = ROOT.parents[1] / "build" / "hd-rgb565-a"
BUILD.mkdir(parents=True, exist_ok=True)
jobs = json.loads((EXPANDED / "jobs-rgb565-a.json").read_text(encoding="utf-8"))
for job in jobs:
    texture_id = job["id"]
    original = Image.open(EXPANDED / "original" / f"{texture_id}.png").convert("RGBA")
    if texture_id in (7581, 7584):
        helper = original.copy()
        helper.paste(helper.crop((0, 1, 61, 65)).transpose(Image.Transpose.ROTATE_180), (0, 1))
        helper.resize((512, 512), Image.Resampling.NEAREST).save(
            EXPANDED / "generated" / f"{texture_id}-upright-support.png")
    raw = Image.open(EXPANDED / "generated" / f"{texture_id}-raw.png").convert("RGBA")
    raw.resize((512, 512), Image.Resampling.LANCZOS).save(BUILD / f"{texture_id}-normalized-view.png")
    result = Image.new("RGBA", (512, 512), (0, 0, 0, 255))
    for region in job["uv_regions"]:
        bounds = [n * 4 for n in region["source_bbox"]]
        patch = raw.crop(region["generated_bbox"])
        if region.get("rotate") == 180:
            patch = patch.transpose(Image.Transpose.ROTATE_180)
        patch = patch.resize((bounds[2] - bounds[0], bounds[3] - bounds[1]), Image.Resampling.LANCZOS)
        result.paste(patch, bounds[:2])
    result.save(BUILD / f"{texture_id}-registered-preview.png")
    (EXPANDED / "generated" / f"{texture_id}-prompt.txt").write_text(job["prompt"] + "\n", encoding="utf-8")
    print(f"{texture_id}: registered preview {result.size}; regions={len(job['uv_regions'])}")

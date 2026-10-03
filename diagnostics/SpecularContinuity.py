"""Regression for the blue car in the 2026-10-03 quicksave.

Copy that save to an isolated lab run as car_glints_quick_20261003, generate
a plan, run it with ProfileFrames.py, then check the captured evidence.
The world is held while the camera sweeps across the reported failure. Each
pose is captured with specular on/off to isolate the highlight from diffuse
lighting, reflections, shadows and screen-space movement of the car.
"""
import argparse
import json
from pathlib import Path


def plan(tag, save):
    steps = [{"command": f"load {save}"}, {"wait": 4},
             {"command": "perfworld 0"}]
    for i in range(17):
        steps += [{"command": f"perfpose {16.138327 + (i-4)*.1:.6f} 9.040683 9.893331 -.464099 -.759356 17.960207"},
                  {"command": "console setvar gfx_specular = 0"},
                  {"screenshot": f"{tag}_{i}_off"},
                  {"command": "console setvar gfx_specular = 1"},
                  {"screenshot": f"{tag}_{i}_on"}]
    return steps


def check(evidence, tag):
    import numpy as np
    from PIL import Image
    roi = (1250, 300, 2200, 860)
    rows = []
    for i in range(17):
        def pixels(state):
            im = Image.open(evidence / f"{tag}_{i}_{state}.bmp").convert("RGB")
            if im.size != (2560, 1440):
                raise ValueError("The car fixture requires 2560x1440")
            return np.asarray(im).astype(np.int16)[roi[1]:roi[3], roi[0]:roi[2]]
        off, on = pixels("off"), pixels("on")
        if off.mean() <= 5:
            raise ValueError("Empty/dark scene")
        delta = np.maximum(on - off, 0)
        rows.append({"offset": round((i-4)*.1, 1), "highlight_sum": int(delta.sum()),
                     "highlight_pixels_gt10": int((delta.max(axis=2) > 10).sum())})
    ratios = [max(a["highlight_sum"], b["highlight_sum"]) /
              max(1, min(a["highlight_sum"], b["highlight_sum"]))
              for a, b in zip(rows, rows[1:])]
    # The broken mask loses nearly the entire hood/fender (~17,000 pixels)
    # and jumps by >6x. The repaired pass changes smoothly by <5% per step.
    return {"tag": tag, "roi": roi, "frames": rows,
            "maximum_adjacent_ratio": max(ratios),
            "passed": max(ratios) < 1.25 and
                      all(r["highlight_pixels_gt10"] > 5000 for r in rows)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("plan", "check"))
    parser.add_argument("path", type=Path)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--save", default="car_glints_quick_20261003")
    args = parser.parse_args()
    for value in (args.tag, args.save):
        if not value or any(c not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-" for c in value):
            parser.error("Invalid tag/save slot")
    if args.mode == "plan":
        with args.path.open("x", encoding="utf-8") as f:
            json.dump(plan(args.tag, args.save), f, indent=2)
    else:
        result = check(args.path, args.tag)
        with (args.path / f"{args.tag}-continuity.json").open("x", encoding="utf-8") as f:
            json.dump(result, f, indent=2)
        print(json.dumps(result, indent=2))
        raise SystemExit(0 if result["passed"] else 1)


if __name__ == "__main__":
    main()

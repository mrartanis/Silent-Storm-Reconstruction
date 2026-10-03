"""Reproduce the user's lamp-shadow reset in the isolated `shadow` fixture.

Generate a plan for ProfileFrames.py, then check its road patch before/after
floor resets. This checks a specific reported shadow, not global sky convergence.
The fixture and camera pose come from the user's 2026-10-03 shadow save.
Pixel checks require Pillow and NumPy. Original save files must remain untouched.
"""
import argparse
import json
from pathlib import Path


def plan(tag, fixture):
    pose = "perfpose 12.871022 7.472300 9.853409 -.962498 -.741287 17.961264"
    steps = [{"command": f"load {fixture}"}, {"wait": 4},
             {"command": "perfworld 0"}, {"command": pose},
             {"command": "perffloor 0"}, {"wait": 4}]
    for i, distance in enumerate((.00001, 1, 8, 24, 96)):
        steps += [{"command": f"perfpan {distance} 0 0"},
                  {"command": f"perfpan {-distance} 0 0"},
                  {"wait": .5}, {"screenshot": f"{tag}_before_{i}"},
                  {"command": "perffloor 1"}, {"command": "perffloor 0"},
                  {"screenshot": f"{tag}_after_{i}"}]
    steps += [{"command": "perfworld 1"}]
    return steps


def check(evidence, tag):
    import numpy as np
    from PIL import Image
    rows = []
    # Road beside the corpse: ground pixels, no units, cursor, or UI.
    roi = (1600, 520, 1650, 550)
    for i in range(5):
        images = [Image.open(evidence / f"{tag}_{step}_{i}.bmp")
                  for step in ("before", "after")]
        if any(im.size != (2560, 1440) for im in images):
            raise ValueError("This fixture regression requires 2560x1440")
        a, b = [np.asarray(im.crop(roi).convert("RGB")).astype(np.int16)
                for im in images]
        delta = np.abs(a - b)
        rows.append({"transition": i, "mean_difference": float(delta.mean()),
                     "max_difference": int(delta.max()),
                     "before_mean": float(a.mean()), "after_mean": float(b.mean())})
    return {"tag": tag, "roi": roi, "transitions": rows,
            "passed": all(r["max_difference"] <= 1 and r["before_mean"] > 10
                          and r["after_mean"] > 10 for r in rows)}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("mode", choices=("plan", "check"))
    p.add_argument("path", type=Path)
    p.add_argument("--tag", required=True)
    p.add_argument("--fixture", default="shadow")
    p.add_argument("--report", type=Path)
    a = p.parse_args()
    if not a.tag or any(c not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-"
                        for c in a.tag):
        p.error("Tag must contain only ASCII letters, digits, '-' and '_'")
    if a.mode == "plan":
        with a.path.open("x", encoding="utf-8") as f:
            json.dump(plan(a.tag, a.fixture), f, indent=2)
    else:
        result = check(a.path, a.tag)
        with (a.report or a.path / f"{a.tag}-transitions.json").open("x", encoding="utf-8") as f:
            json.dump(result, f, indent=2)
        print(json.dumps(result, indent=2))
        raise SystemExit(0 if result["passed"] else 1)


if __name__ == "__main__":
    main()

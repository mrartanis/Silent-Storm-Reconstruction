"""Compare moving-camera lighting with a settled frame in the shadows2 fixture.

Run generated plans with ProfileFrames.py in an isolated lab run. The camera
oscillates before EVERY frame, unlike the earlier between-command pan probes.
The subpixel displacement triggers recovery without changing the visible view.
World clocks are held; small edge/specular differences are still allowed.
"""
import argparse
import json
from pathlib import Path


def plan(tag):
    pose = "perfpose 17.371016 20.343037 9.893331 -.687098 -.759942 17.960844"
    steps = [{"command": "load shadows2"}, {"wait": 4},
             {"command": "perfworld 0"}, {"command": pose},
             {"command": "perfoscillate 0 0 0 0"}, {"wait": 10},
             {"screenshot": f"{tag}_idle"},
             {"command": "perfoscillate .00001 0 0 1200"}]
    for i in range(3):
        steps += [{"command": "perfmotionstatus"},
                  {"screenshot": f"{tag}_during{i}"}, {"wait": .5}]
    steps += [{"capture": f"{tag}_fps", "frames": 120},
              {"command": "perfoscillate 0 0 0 0"},
              {"screenshot": f"{tag}_stop"}, {"wait": 4},
              {"screenshot": f"{tag}_settled"},
              {"command": "perfoscillate 8 0 0 240"}, {"wait": 1},
              {"command": "perfmotionstatus"},
              {"screenshot": f"{tag}_large_motion"},
              {"command": "perfoscillate 0 0 0 0"},
              {"screenshot": f"{tag}_large_return"},
              {"command": "perffloor 1"}, {"command": "perffloor 0"},
              {"screenshot": f"{tag}_floor_return"},
              {"command": "perfzoom 40"}, {"command": pose},
              {"screenshot": f"{tag}_zoom_return"}]
    return steps


def check(evidence, tag, observations):
    import numpy as np
    from PIL import Image
    def pixels(suffix):
        im = Image.open(evidence / f"{tag}_{suffix}.bmp").convert("RGB")
        if im.size != (2560, 1440):
            raise ValueError("The fixture requires 2560x1440")
        return np.asarray(im).astype(np.int16)
    reference = pixels("idle")
    if reference[50:1180].mean() <= 5:
        raise ValueError("Empty/dark scene")
    rows = []
    # Clear ground through the front arch, away from edges, units, and cursor.
    roi = (1140, 980, 1220, 1040)
    for suffix in ("during0", "during1", "during2", "stop", "settled",
                   "large_return", "floor_return", "zoom_return"):
        delta = np.abs(pixels(suffix) - reference)
        scene = delta[50:1180]
        floor = delta[roi[1]:roi[3], roi[0]:roi[2]]
        rows.append({"frame": suffix, "scene_mean": float(scene.mean()),
                     "scene_pixels_gt10": int((scene.max(axis=2) > 10).sum()),
                     "floor_mean": float(floor.mean()), "floor_max": int(floor.max())})
    logs = json.loads(observations.read_text(encoding="utf-8"))
    motion = [item["result"] for item in logs if item.get("command") == "perfmotionstatus"]
    import re
    remaining = [int(re.search(r"remaining=(\d+)", entry)[1]) for entry in motion]
    # Broad lighting patches changed ~100,000 pixels in the old build.
    # Actual raster/specular edges with a subpixel move affect <400 pixels.
    return {"tag": tag, "floor_roi": roi, "frames": rows,
            "motion_remaining": remaining,
            "passed": len(remaining) == 4 and all(n > 0 for n in remaining) and
                      all(r["scene_mean"] < .05 and r["scene_pixels_gt10"] < 1000 and
                          r["floor_mean"] < .01 and r["floor_max"] <= 1 for r in rows)}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("mode", choices=("plan", "check"))
    p.add_argument("path", type=Path)
    p.add_argument("--tag", required=True)
    p.add_argument("--observations", type=Path)
    a = p.parse_args()
    if not a.tag or any(c not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-" for c in a.tag):
        p.error("Invalid tag")
    if a.mode == "plan":
        with a.path.open("x", encoding="utf-8") as f:
            json.dump(plan(a.tag), f, indent=2)
    else:
        if a.observations is None:
            p.error("--observations is required for motion validation")
        result = check(a.path, a.tag, a.observations)
        with (a.path / f"{a.tag}-motion.json").open("x", encoding="utf-8") as f:
            json.dump(result, f, indent=2)
        print(json.dumps(result, indent=2))
        raise SystemExit(0 if result["passed"] else 1)


if __name__ == "__main__":
    main()

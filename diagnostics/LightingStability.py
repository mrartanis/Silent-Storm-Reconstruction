"""Generate a camera-jitter plan or check repeated-pose screenshots.

Use ProfileFrames.py in an isolated New-LabRun directory to execute the plan.
S2_LIGHTING_MOVING_SKY_SAMPLES=1 reproduces the former rotating-sample path.
Checking pixels requires Pillow and NumPy (bundled workspace Python has both).
"""
import argparse
import json
from pathlib import Path


def plan(tag, fixture):
    result = [{"command": f"load {fixture}"}, {"wait": 4}]
    options = {"resolution": "2560x1440", "antialiasing": 1,
               "anisotropic_filter": 16, "vsync": 0, "shadows": 1,
               "specular": 1, "blur_sun": 1}
    result += [{"command": f"console setvar gfx_{key} = {value}"}
               for key, value in options.items()]
    result += [{"command": "console gfx_update"}, {"wait": 2},
               {"command": "perfzoom 80"}, {"wait": 1},
               {"command": "perfpan -7 -23 0.4"}, {"wait": 1},
               {"command": "perfworld 0"}, {"wait": 1},
               {"command": "displaystatus"}, {"command": "camerastatus"}]
    for index in range(12):
        delta = ".00001" if index % 2 == 0 else "-.00001"
        result += [{"command": f"perfpan {delta} 0 0"}, {"wait": .15},
                   {"screenshot": f"{tag}_{index:02d}"}]
    result += [{"command": "camerastatus"}, {"command": "perfworld 1"}]
    return result


def check(evidence, tag):
    import numpy as np
    from PIL import Image
    reference = None
    rows = []
    # Even frames share a pose. Odd frames have a real raster displacement.
    for index in range(0, 12, 2):
        path = evidence / f"{tag}_{index:02d}.bmp"
        image = np.asarray(Image.open(path)).astype(np.int16)
        if image.shape[:2] != (1440, 2560):
            raise ValueError(f"Unexpected screenshot size: {path}: {image.shape}")
        scene = image[40:1100, :, :3]
        if not scene.any():
            raise ValueError(f"Empty scene screenshot: {path}")
        if reference is None:
            reference = scene
        delta = np.abs(scene - reference)
        rows.append({"frame": index, "mean_difference": float(delta.mean()),
                     "max_difference": int(delta.max()),
                     "changed_pixels": int((delta.max(axis=2) > 5).sum())})
    # Allow isolated 8-bit rounding at triangle edges, but no pixel changing
    # by >5 levels and no widespread low-amplitude change in the scene.
    return {"tag": tag, "same_pose_frames": rows,
            "passed": all(row["changed_pixels"] == 0 and row["mean_difference"] <= .0001
                          for row in rows)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_subparsers(dest="mode", required=True)
    generate = modes.add_parser("plan")
    generate.add_argument("output", type=Path)
    generate.add_argument("--fixture", default="stage6_entered_mission_fixture")
    verify = modes.add_parser("check")
    verify.add_argument("evidence", type=Path)
    verify.add_argument("--report", type=Path)
    for mode in (generate, verify):
        mode.add_argument("--tag", required=True)
    args = parser.parse_args()
    if not args.tag or any(c not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-" for c in args.tag):
        parser.error("Tag must contain only ASCII letters, numbers, '-' and '_'")
    if args.mode == "plan":
        with args.output.open("x", encoding="utf-8") as stream:
            json.dump(plan(args.tag, args.fixture), stream, indent=2)
    else:
        report = check(args.evidence, args.tag)
        destination = args.report or args.evidence / f"{args.tag}-stability.json"
        with destination.open("x", encoding="utf-8") as stream:
            json.dump(report, stream, indent=2)
        print(json.dumps(report, indent=2))
        raise SystemExit(0 if report["passed"] else 1)


if __name__ == "__main__":
    main()

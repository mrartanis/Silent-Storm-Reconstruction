"""Inspect real texture residency across zoom, motion and floor changes.

Run generated plans with ProfileFrames.py and S2_TEXTURE_DIAGNOSTICS=1.
Full mode must keep both terrain layers at 256 and stop LR loads/evictions
after the first settled snapshot. The legacy mode is a comparison control.
"""
import argparse
import json
import re
from pathlib import Path


def plan(tag, legacy):
    steps = [{"command": f"console setvar gfx_texture_streaming = {int(legacy)}"},
             {"command": "load stage6_entered_mission_fixture"}, {"wait": 4},
             {"command": "perfworld 0"}, {"command": "perfzoom 35"},
             {"command": "perfpan -7 -23 .4"}, {"wait": 4},
             {"command": "texturestatus"}, {"screenshot": f"{tag}_close"}]
    for rod in (35, 60, 80):
        steps += [{"command": f"perfzoom {rod}"}, {"wait": 3},
                  {"command": "texturestatus"}, {"screenshot": f"{tag}_{rod}_idle"},
                  {"capture": f"{tag}_{rod}_idle", "frames": 120,
                   "width": 2560, "height": 1440},
                  {"command": "perfoscillate .00001 0 0 1200"},
                  {"command": "texturestatus"},
                  {"capture": f"{tag}_{rod}_moving", "frames": 120},
                  {"command": "texturestatus"}, {"screenshot": f"{tag}_{rod}_moving"},
                  {"command": "perfoscillate 0 0 0 0"}, {"wait": 3},
                  {"command": "texturestatus"}]
    return steps + [{"command": "perfzoom 35"}, {"command": "perffloor 1"},
                    {"command": "perffloor 0"}, {"wait": 3},
                    {"command": "texturestatus"}, {"screenshot": f"{tag}_return"}]


def check(path, legacy):
    observations = json.loads(path.read_text(encoding="utf-8"))
    rows = []
    for item in observations:
        if item.get("command") == "texturestatus":
            line = next(s for s in item["result"].splitlines() if "[harness] textures " in s)
            rows.append({k: int(v) for k, v in re.findall(r"(\w+)=(\d+)", line)})
    valid = len(rows) == 14 and all(r["enabled"] == 1 and r["streaming"] == int(legacy)
                                  and r["terrain_high"] + r["terrain_low"] > 0 for r in rows)
    if not legacy and valid:
        valid = all(r["terrain_high"] > 0 and r["bump_high"] > 0 and
                    r["terrain_low"] == r["terrain_fake"] == r["bump_low"] == r["files_fake"] == 0
                    for r in rows)
        valid = valid and all(r[k] == rows[0][k] for r in rows for k in
                             ("evictions", "generated128", "stress_fallbacks", "pending_fallbacks", "file_low_loads"))
    return {"legacy": legacy, "snapshots": rows, "passed": valid}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("action", choices=("plan", "check"))
    p.add_argument("path", type=Path)
    p.add_argument("--legacy", action="store_true")
    p.add_argument("--tag", default="texture_residency")
    p.add_argument("--report", type=Path)
    a = p.parse_args()
    if not re.fullmatch(r"[A-Za-z0-9_-]+", a.tag):
        p.error("Invalid tag")
    result = plan(a.tag, a.legacy) if a.action == "plan" else check(a.path, a.legacy)
    destination = a.path if a.action == "plan" else a.report
    if destination:
        with destination.open("x", encoding="utf-8") as f:
            json.dump(result, f, indent=2)
    print(json.dumps(result, indent=2))
    if a.action == "check":
        raise SystemExit(0 if result["passed"] else 1)


if __name__ == "__main__":
    main()

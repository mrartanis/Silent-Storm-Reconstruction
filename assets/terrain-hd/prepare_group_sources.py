#!/usr/bin/env python3
"""Verify independent group sources without modifying shared HD metadata."""
import argparse
from collections import Counter
import json
from pathlib import Path

from prepare_sources import prepare


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--historical", required=True, type=Path)
    parser.add_argument("--release", required=True, type=Path)
    parser.add_argument("--groups", type=Path, default=Path(__file__).parent / "expanded/groups")
    parser.add_argument("--first-batches", action="store_true", help="Also export the reviewed-scope initial ID subsets; images still require examination")
    args = parser.parse_args()
    destination = args.groups / "source-queues"
    destination.mkdir(parents=True, exist_ok=True)
    summary = {"generation_started": False, "examined_images": False,
               "readiness": "RGBA sources verified; each image and UV/atlas layout still require private inspection before imagegen",
               "groups": {}}
    for source in sorted((args.groups / "candidates").glob("*.json")):
        candidates = json.loads(source.read_text(encoding="utf-8"))
        if not candidates:
            continue
        records = prepare(candidates, args.historical, args.release, destination / source.name, append=False)
        statuses = dict(Counter(record["status"] for record in records))
        summary["groups"][source.stem] = {"records": len(records), "statuses": statuses,
            "fallback_ids": [{"id": record["id"], "status": record["status"], "reason": record.get("reason")}
                             for record in records if record["status"] != "pending"]}
        print(source.stem, len(records), statuses)
    (destination / "source-check-summary.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    if args.first_batches:
        first = args.groups / "first-batches"
        first.mkdir(exist_ok=True)
        for group, ids in [("characters-clothing", [798, 800, 915, 917]),
                           ("equipment", [657, 664, 665, 666, 667, 668, 669])]:
            records = {record["id"]: record for record in json.loads((destination / (group + ".json")).read_text(encoding="utf-8"))}
            # Do not re-export records that the coordinator has already registered.
            selected = [records[i] for i in ids if i in records and records[i]["status"] == "pending"]
            (first / (group + ".json")).write_text(json.dumps(selected, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()

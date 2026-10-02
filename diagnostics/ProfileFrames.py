"""Capture opt-in frame scopes in an isolated harness run; never attach to user games.

Plan: JSON array of {command: str}, {wait: seconds}, or {capture: tag, frames: int}.
CPU scopes are inclusive. bgfx metrics describe earlier asynchronous frames.
"""
import argparse
import csv
import json
import os
from pathlib import Path
import statistics
import subprocess
import time


def summarize(path):
    with path.open(newline="") as f:
        all_rows = list(csv.DictReader(f))
    rows = [r for r in all_rows if int(r["presents"]) == 1]
    if not rows:
        raise ValueError(f"No presented frames in {path}")
    result = {"file": str(path.resolve()), "frames": len(rows),
              "discarded": len(all_rows)-len(rows)}
    for name in rows[0]:
        values = sorted(float(r[name]) for r in rows if float(r[name]) >= 0)
        if not values:
            continue
        result[name] = {"mean": statistics.mean(values),
                        "median": statistics.median(values),
                        "p95": values[min(len(values)-1, int(.95*len(values)))]}
    result["fps"] = 1000/result.get("frame_interval_ms", result["frame_ms"])["mean"]
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("run", type=Path)
    p.add_argument("plan", type=Path)
    p.add_argument("--cfg", default="cfg/lab-no-intro.cfg")
    a = p.parse_args()
    run = a.run.resolve()
    if not (run/"evidence/run.json").exists():
        p.error("Requires a prepared isolated New-LabRun directory")
    channel = run/"game/_harness_cmd.txt"
    log = run/"game/_saveload.log"
    if channel.exists():
        p.error("Command channel already occupied")
    evidence = run/"evidence"
    env = os.environ.copy()
    env["S2_USER_DATA_DIR"] = str(run/"user-data")
    stream = (evidence/(a.plan.stem+"-process.log")).open("wb")
    process = subprocess.Popen([str(run/"game/Game.exe"), "-windowed", "-harness",
                                "-harness-active", "-cfg", a.cfg],
                               cwd=run/"game", env=env, stdout=stream, stderr=stream)
    (evidence/"perf-process.pid").write_text(str(process.pid))
    observations = []

    def command(value):
        deadline = time.monotonic()+180
        offset = log.stat().st_size if log.exists() else 0
        if channel.exists():
            raise RuntimeError("Channel busy")
        temp = channel.with_suffix(".tmp")
        temp.write_text(value+"\n", encoding="ascii")
        temp.replace(channel)
        while True:
            if process.poll() is not None:
                if value == "quit": return ""
                raise RuntimeError(f"Game exited: {process.returncode}")
            text = log.read_bytes()[offset:].decode("latin1") if log.exists() else ""
            if not channel.exists() and "[harness] cmd: "+value in text:
                time.sleep(.15)
                text = log.read_bytes()[offset:].decode("latin1")
                observations.append({"command": value, "result": text})
                return text
            if time.monotonic() > deadline:
                raise TimeoutError(value)
            time.sleep(.05)

    summaries = []
    try:
        time.sleep(3)
        command("displaystatus")
        for step in json.loads(a.plan.read_text(encoding="utf-8")):
            print(step, flush=True)
            if "command" in step:
                command(step["command"])
            elif "wait" in step:
                time.sleep(step["wait"])
            elif "capture" in step:
                tag = step["capture"]
                frames = step.get("frames", 180)
                response = command(f"perf {tag} {frames}")
                if "perf started=1" not in response:
                    raise RuntimeError(response)
                deadline = time.monotonic()+240
                while "remaining=0 " not in command("perfstatus"):
                    if time.monotonic() > deadline: raise TimeoutError(tag)
                    time.sleep(.5)
                path = run/"game"/f"_perf_{tag}.csv"
                summary = summarize(path)
                if summary["frames"] != frames:
                    raise RuntimeError(f"Capture includes skipped frames: {summary}")
                summaries.append(summary)
                print(json.dumps({"tag": tag, "fps": summary["fps"],
                                  "frame_ms": summary["frame_ms"]}), flush=True)
            else:
                raise ValueError(step)
    finally:
        (evidence/(a.plan.stem+"-observations.json")).write_text(
            json.dumps(observations, indent=2), encoding="utf-8")
        (evidence/(a.plan.stem+"-summary.json")).write_text(
            json.dumps(summaries, indent=2), encoding="utf-8")
        if process.poll() is None:
            command("quit")
            process.wait(timeout=30)
        stream.close()
        print("Game exited", process.returncode, flush=True)


if __name__ == "__main__":
    main()

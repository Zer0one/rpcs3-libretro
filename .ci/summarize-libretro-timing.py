#!/usr/bin/env python3
"""Summarize RPCS3 timing windows; CPU wall times are not GPU timings."""
import argparse
import json
import re
import statistics
from pathlib import Path


def read_windows(path):
    windows = []
    for line in path.read_text(errors="replace").splitlines():
        if "[RPCS3 Timing]" not in line:
            continue
        row = {k: float(v) for k, v in re.findall(r"([a-z_0-9]+)=(-?\d+(?:\.\d+)?)", line)}
        if "pause_wait_ms" not in row:  # A log cut short during a crash/write.
            continue
        stamp = re.search(r"(\d+):(\d+):(\d+\.\d+)\s+LibretroTiming:", line)
        if stamp:
            row["log_seconds"] = int(stamp[1]) * 3600 + int(stamp[2]) * 60 + float(stamp[3])
        windows.append(row)
    return windows


def summarize(rows):
    if not rows:
        raise ValueError("No complete timing windows in selected range")
    seconds = sum(r["window_s"] for r in rows)
    ready_key = "vk_ready_hz" if "vk_ready_hz" in rows[0] else "ready_hz"
    # Frontend-owned counters share an exact window; cross-thread durations
    # may straddle a boundary and must not be added into a CPU/GPU budget.
    for r in rows:
        if abs(r["shown_hz"] * r["window_s"] + r["dup"] - r["runs"]) > .02:
            raise ValueError("Shown and duplicate counters do not reconcile with runs")
        if r["audio_accepted"] > r["audio_offered"]:
            raise ValueError("Accepted audio exceeds offered audio")
    result = {
        "windows": len(rows), "seconds": seconds,
        "frontend_hz": sum(r["runs"] for r in rows) / seconds,
        "completed_frame_hz": sum(r[ready_key] * r["window_s"] for r in rows) / seconds,
        "shown_frame_hz": sum(r["shown_hz"] * r["window_s"] for r in rows) / seconds,
        "median_window_completed_hz": statistics.median(r[ready_key] for r in rows),
        "min_window_completed_hz": min(r[ready_key] for r in rows),
        "duplicates": sum(r["dup"] for r in rows), "dropped": sum(r["dropped"] for r in rows),
        "steps": sum(r["steps"] for r in rows),
        "median_window_run_p95_ms": statistics.median(r["run_p95_ms"] for r in rows),
        "audio_offered_hz": sum(r["audio_offered"] for r in rows) / seconds,
        "audio_accepted_hz": sum(r["audio_accepted"] for r in rows) / seconds,
        "units": "CPU milliseconds per wall-clock second; overlapping threads/scopes, not additive",
    }
    for key in ["video_cb_ms", "audio_pull_ms", "audio_cb_ms", "queue_wait_ms", "queue_hold_ms", "acquire_ms", "fence_wait_ms", "pause_wait_ms", "frontend_wait_ms", "handoff_submit_ms"]:
        if all(key in r for r in rows):
            result[key + "_per_second"] = sum(r[key] for r in rows) / seconds
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("log", type=Path)
    p.add_argument("--from-seconds", type=float, default=0, help="RPCS3 log timestamp lower bound")
    p.add_argument("--to-seconds", type=float, default=float("inf"))
    p.add_argument("--output", type=Path)
    a = p.parse_args()
    rows = read_windows(a.log)
    if a.from_seconds or a.to_seconds != float("inf"):
        rows = [r for r in rows if a.from_seconds <= r.get("log_seconds", -1) <= a.to_seconds]
    result = {"source": str(a.log), "summary": summarize(rows), "raw_windows": rows}
    text = json.dumps(result, indent=2) + "\n"
    if a.output:
        a.output.write_text(text)
    else:
        print(text, end="")


if __name__ == "__main__":
    main()

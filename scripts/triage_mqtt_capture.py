#!/usr/bin/env python3
"""Summarise an INF2004 Pico MQTT monitor capture (plain text or .gz).

This is diagnostic evidence, not proof of physical motion or a PASS/FAIL.
Use only captures made from an actual robot at a recorded firmware revision.
"""
import argparse
from collections import Counter
import gzip
import json
from pathlib import Path
import re

TELEMETRY = re.compile(r"^TELEMETRY #(?P<seq>\d+) state=(?P<state>[^ ]+).*?\bspeed L=(?P<left>-?[\d.]+) R=(?P<right>-?[\d.]+) mm/s.*?\bhumps=(?P<humps>\d+).*?\bfront=(?P<front>no echo|[\d.]+ mm).*?\bbarcode=(?P<barcode>[^ ]+)")
WHEEL_LIMIT_MMPS = 500.0


def open_capture(path):
    return gzip.open(path, "rt", encoding="utf-8", errors="replace") if str(path).endswith(".gz") else open(path, "r", encoding="utf-8", errors="replace")


def analyse(lines, limit=WHEEL_LIMIT_MMPS):
    states, events = Counter(), Counter()
    anomalies = []
    totals = Counter()
    last_state = None
    first_humps = None
    last_humps = None
    first_seq = None
    last_seq = None
    observed_speeds = []
    for number, original in enumerate(lines, 1):
        line = original.strip()
        if "TELEMETRY #" in line and not line.startswith("TELEMETRY #"):
            line = line[line.index("TELEMETRY #"):]
        if "EVENT " in line and not line.startswith("EVENT "):
            line = line[line.index("EVENT "):]
        if line.startswith("EVENT "):
            name = line.split(":", 1)[0].partition(" ")[2]
            events[name] += 1
        if "barcode capture overflow" in line.lower():
            totals["barcode_overflow_lines"] += 1
        if "IMU calibration failed" in line:
            totals["imu_calibration_failure_lines"] += 1
        if "reset by watchdog" in line.lower():
            totals["watchdog_reset_messages"] += 1
        if line.startswith("WHEEL "):
            totals["wheel_trace_frames"] += 1
            try:
                trace = json.loads(line[6:])
                for key in ("measured_left", "measured_right"):
                    value = trace.get(key)
                    if isinstance(value, (int, float)) and abs(value) > limit:
                        anomalies.append({"line": number, "reason": "wheel_trace_speed_over_limit", "field": key, "value": value})
            except (ValueError, TypeError):
                totals["undecodable_wheel_traces"] += 1
        if line.startswith("LINE "):
            totals["line_trace_frames"] += 1
        match = TELEMETRY.search(line)
        if not match:
            continue
        totals["telemetry_frames"] += 1
        d = match.groupdict()
        states[d["state"]] += 1
        if last_state is not None and d["state"] != last_state:
            totals["state_changes"] += 1
            if d["state"] == "STOPPED":
                anomalies.append({"line": number, "reason": "entered_STOPPED", "from": last_state})
        last_state = d["state"]
        if d["front"] == "no echo":
            totals["telemetry_no_echo"] += 1
        speed_left, speed_right = float(d["left"]), float(d["right"])
        observed_speeds.append((speed_left, speed_right))
        if abs(speed_left) > limit or abs(speed_right) > limit:
            anomalies.append({"line": number, "reason": "averaged_speed_over_limit", "left": speed_left, "right": speed_right})
        last_humps = int(d["humps"])
        if first_humps is None:
            first_humps = last_humps
        seq = int(d["seq"])
        if first_seq is None:
            first_seq = seq
        last_seq = seq

    return {
        "counts": dict(totals),
        "states": dict(states),
        "events": dict(events),
        "first_and_last_hump_counts": [first_humps, last_humps],
        "first_and_last_telemetry_sequence": [first_seq, last_seq],
        "max_abs_telemetry_speed_mmps": max((abs(x) for pair in observed_speeds for x in pair), default=None),
        "anomalies": anomalies,
        "limitations": [
            "A no-echo telemetry report alone is not a stop cause.",
            "A speed spike may reflect encoder noise, wrap, missing telemetry intervals or actual motion; inspect WHEEL frames and video.",\n            "Terminal/dashboard speed is estimated from MQTT arrival times, not sensor timestamps; burst or delayed delivery makes it unreliable. Use WHEEL sampled_ms and video for confirmation.",
            "A telemetry barcode display may persist without being a new read; it is not a per-pass event count.",
            "Missing frames/events in this capture do not prove the robot failed to produce them.",
            "Reset cause and safety/stopping distance require independent physical evidence.",
        ],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path, help="Saved mqtt_client.py monitor output (.txt or .txt.gz)")
    parser.add_argument("--speed-limit-mmps", type=float, default=WHEEL_LIMIT_MMPS)
    parser.add_argument("--max-anomalies", type=int, default=20)
    args = parser.parse_args()
    if args.speed_limit_mmps <= 0 or args.max_anomalies < 0:
        parser.error("speed limit must be positive and max anomalies nonnegative")
    with open_capture(args.capture) as handle:
        result = analyse(handle, limit=args.speed_limit_mmps)
    result["anomalies_total"] = len(result["anomalies"])
    result["anomalies"] = result["anomalies"][:args.max_anomalies]
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

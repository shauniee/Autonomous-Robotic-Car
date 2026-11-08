#!/usr/bin/env python3
"""Summarise an INF2004 Pico MQTT monitor capture (plain text or .gz).

Reads the compact form from compact_log.py or a raw --raw capture.
Diagnostic evidence only: telemetry speed comes from MQTT arrival times, a
no-echo report is not a stop cause, and missing frames do not prove the robot
failed to send them.
"""
import argparse
from collections import Counter
import gzip
import json
import math
from pathlib import Path
import re

REPEATED = re.compile(r"^(?P<kind>\S+) repeated x(?P<count>\d+)")
WHEEL_KV = re.compile(r"^WHEEL t=(?P<t>\d+) .*?speed=(?P<left>[^/ ]+)/(?P<right>\S+)")
TELEMETRY = re.compile(r"^TELEMETRY #(?P<seq>\d+) state=(?P<state>[^ ]+).*?\bhumps=(?P<humps>\d+).*?\bfront=(?P<front>no echo|[\d.]+ mm).*?\bbarcode=(?P<barcode>[^ ]+)")
SPEED = re.compile(r"\bspeed L=(-?\d+(?:\.\d+)?) R=(-?\d+(?:\.\d+)?) mm/s")
WHEEL_LIMIT_MMPS = 500.0


def open_capture(path):
    return gzip.open(path, "rt", encoding="utf-8", errors="replace") if str(path).endswith(".gz") else open(path, "r", encoding="utf-8", errors="replace")


def wheel_speeds(line):
    """(sampled_ms, {field: speed}) from a JSON or compact WHEEL line."""
    body = line[6:]
    if body.startswith("{"):
        trace = json.loads(body)
        if not isinstance(trace, dict):
            raise ValueError("wheel record must be an object")
        return trace.get("sampled_ms"), {k: trace.get(k) for k in ("measured_left", "measured_right")}
    match = WHEEL_KV.match(line)
    if not match:
        raise ValueError("wheel record not recognised")
    def value(text):
        return None if text == "-" else float(text)
    return int(match["t"]), {"measured_left": value(match["left"]), "measured_right": value(match["right"])}


def flag_counts(line, totals, count):
    """Per-telemetry-line counters; a folded repeat counts as `count` lines."""
    if "barcode capture overflow" in line.lower():
        totals["barcode_overflow_lines"] += count
    if "IMU calibration failed" in line:
        totals["imu_calibration_failure_lines"] += count
    if "front=no echo" in line:
        totals["telemetry_no_echo"] += count
    if not SPEED.search(line):
        totals["telemetry_speed_unavailable"] += count


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
    wheel_peak = None
    transitions = []
    last_telemetry = ""
    for number, original in enumerate(lines, 1):
        line = original.strip()
        record = re.search(r"\b(?:TELEMETRY #|EVENT |WHEEL |LINE )", line)
        if record:
            line = line[record.start():]
        repeated = REPEATED.match(line)
        if repeated:
            count = int(repeated["count"])
            kind = repeated["kind"]
            if kind == "WHEEL":
                totals["wheel_trace_frames"] += count
            elif kind == "LINE":
                totals["line_trace_frames"] += count
            elif kind == "TELEMETRY" and last_state is not None:
                totals["telemetry_frames"] += count
                states[last_state] += count
                flag_counts(last_telemetry, totals, count)
            continue
        if line.startswith("EVENT "):
            name = line.split(":", 1)[0].partition(" ")[2]
            events[name] += 1
        if line.startswith("WHEEL "):
            totals["wheel_trace_frames"] += 1
            try:
                sampled_ms, speeds = wheel_speeds(line)
                for key, value in speeds.items():
                    if value is None:
                        continue
                    if type(value) not in (int, float) or not math.isfinite(value):
                        raise ValueError("invalid wheel speed")
                    if wheel_peak is None or abs(value) > abs(wheel_peak["value"]):
                        wheel_peak = {"line": number, "sampled_ms": sampled_ms, "field": key, "value": value}
                    if abs(value) > limit:
                        anomalies.append({"line": number, "reason": "wheel_trace_speed_over_limit", "field": key, "value": value})
            except (ValueError, TypeError):
                totals["undecodable_wheel_traces"] += 1
        if line.startswith("LINE "):
            totals["line_trace_frames"] += 1
        match = TELEMETRY.search(line)
        if not match:
            continue
        totals["telemetry_frames"] += 1
        flag_counts(line, totals, 1)
        last_telemetry = line
        d = match.groupdict()
        states[d["state"]] += 1
        if last_state is not None and d["state"] != last_state:
            totals["state_changes"] += 1
            transitions.append({"line": number, "from": last_state, "to": d["state"]})
            if d["state"] in ("STOPPED", "FAULT", "REACQUIRE"):
                anomalies.append({"line": number, "reason": "entered_" + d["state"], "from": last_state})
        last_state = d["state"]
        speed = SPEED.search(line)
        if speed:
            speed_left, speed_right = map(float, speed.groups())
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
        "state_transitions": transitions,
        "peak_wheel_speed_mmps": wheel_peak,
        "first_and_last_hump_counts": [first_humps, last_humps],
        "first_and_last_telemetry_sequence": [first_seq, last_seq],
        "max_abs_telemetry_speed_mmps": max((abs(x) for pair in observed_speeds for x in pair), default=None),
        "anomalies": anomalies,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path, help="Saved mqtt_client.py monitor output (.txt or .txt.gz)")
    parser.add_argument("--speed-limit-mmps", type=float, default=WHEEL_LIMIT_MMPS)
    parser.add_argument("--max-anomalies", type=int, default=20)
    args = parser.parse_args()
    if not math.isfinite(args.speed_limit_mmps) or args.speed_limit_mmps <= 0 or args.max_anomalies < 0:
        parser.error("speed limit must be positive and max anomalies nonnegative")
    with open_capture(args.capture) as handle:
        result = analyse(handle, limit=args.speed_limit_mmps)
    result["anomalies_total"] = len(result["anomalies"])
    result["anomalies"] = result["anomalies"][:args.max_anomalies]
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

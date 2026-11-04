#!/usr/bin/env python3
"""Score recorded physical line-tracking trials without inventing a pass rate.

CSV columns and physical evidence procedure: testing/README.md#qualifying-lap-records.
success is PASS or FAIL; leave unperformed trials out of the CSV.
Separately score each exact firmware image, trace setting and lighting condition.
"""
import argparse
from collections import defaultdict
import csv
import math
from pathlib import Path
import re

POSITIVE = ("lap_complete", "barcode_ok", "navigation_ok", "hump_count_ok")
NEGATIVE = ("hand_contact", "reacquire", "stopped_fault", "restarted")
MEASUREMENTS = ("course_speed_mmps", "max_height_error_mm", "height_limit_mm")
FIELDS = ("trial_id", "firmware_sha", "uf2_sha256", "trace", "lighting",
          "success", "evidence", "notes") + POSITIVE + NEGATIVE + MEASUREMENTS


def course_speed():
    """The setting in robot_config.h: one source, so changing it needs no edit here."""
    config = Path(__file__).resolve().parents[1] / "app_program/common/robot_config.h"
    return float(re.search(r"#define COURSE_SPEED_MMPS (\d+)u", config.read_text()).group(1))


def validate_result(row, number):
    """A human supplies the measurements; missing evidence cannot become PASS."""
    passed = row["success"].strip().upper() == "PASS"
    for key in POSITIVE + NEGATIVE:
        value = row[key].strip().lower()
        if value not in ("yes", "no", ""):
            raise ValueError(f"CSV row {number}: {key} must be yes, no or blank")
        if passed and value != ("yes" if key in POSITIVE else "no"):
            raise ValueError(f"CSV row {number}: PASS contradicts or omits {key}")
    values = {}
    for key in MEASUREMENTS:
        raw = row[key].strip()
        if not raw and not passed:
            continue  # An interrupted failed lap may never reach the hump.
        value = float(raw)
        if not math.isfinite(value) or value < 0:
            raise ValueError(f"CSV row {number}: {key} must be finite and nonnegative")
        values[key] = value
    if passed and (values["course_speed_mmps"] != course_speed() or
                   values["max_height_error_mm"] > values["height_limit_mm"]):
        raise ValueError(f"CSV row {number}: PASS violates speed or height limit")


def summarize(rows, target=0.95, minimum=20):
    groups = defaultdict(list)
    used = set()
    for number, row in enumerate(rows, 2):
        if any(row.get(key) is None for key in FIELDS):
            raise ValueError(f"CSV row {number}: missing fields; see testing/README.md")
        rid = row["trial_id"].strip()
        sha = row["firmware_sha"].strip()
        image = row["uf2_sha256"].strip()
        trace = row["trace"].strip()
        lighting = row["lighting"].strip()
        result = row["success"].strip().upper()
        if not all((rid, sha, image, lighting, row["evidence"].strip())) or trace not in ("0", "1") or result not in ("PASS", "FAIL"):
            raise ValueError(f"CSV row {number}: missing trial/profile fields or success must be PASS/FAIL")
        key = (sha, image, trace, lighting)
        if rid in used:
            raise ValueError(f"CSV row {number}: duplicate trial_id")
        used.add(rid)
        validate_result(row, number)
        groups[key].append(result)
    output = []
    for (sha, image, trace, lighting), outcomes in sorted(groups.items()):
        passed = outcomes.count("PASS")
        attempts = len(outcomes)
        rate = passed / attempts
        output.append({
            "firmware_sha": sha,
            "uf2_sha256": image,
            "trace": trace,
            "lighting": lighting,
            "passes": passed,
            "attempts": attempts,
            "success_rate_pct": round(100 * rate, 2),
            "target_met_in_sample": attempts >= minimum and rate >= target,
        })
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_file", type=Path)
    parser.add_argument("--target", type=float, default=0.95)
    parser.add_argument("--minimum", type=int, default=20)
    args = parser.parse_args()
    if not (0.0 < args.target <= 1.0) or args.minimum < 1:
        parser.error("target must be in (0,1] and minimum >= 1")
    try:
        with args.csv_file.open(newline="", encoding="utf-8-sig") as file:
            reader = csv.DictReader(file)
            if not set(FIELDS).issubset(reader.fieldnames or []):
                parser.error(f"CSV needs columns: {', '.join(FIELDS)}")
            scored = summarize(reader, args.target, args.minimum)
    except (OSError, ValueError) as error:
        parser.error(str(error))
    if not scored:
        print("No recorded physical trials; no rate to report.")
        return 0
    for row in scored:
        status = "SAMPLE TARGET MET" if row["target_met_in_sample"] else "SAMPLE TARGET NOT YET MET"
        print(f"{row['firmware_sha']} {row['uf2_sha256']} TRACE={row['trace']} lighting={row['lighting']}: "
              f"{row['passes']}/{row['attempts']} = {row['success_rate_pct']:.2f}% {status}")
    print("Observed sample rate only; no statistical or physical certification is implied.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

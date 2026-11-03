#!/usr/bin/env python3
"""Score recorded physical line-tracking trials without inventing a pass rate.

CSV columns: trial_id,firmware_sha,uf2_sha256,trace,success,notes
success is PASS or FAIL; leave unperformed trials out of the CSV.
Separately score each exact firmware image and trace setting.
"""
import argparse
from collections import defaultdict
import csv
from pathlib import Path

FIELDS = ("trial_id", "firmware_sha", "uf2_sha256", "trace", "success", "notes")


def summarize(rows, target=0.95, minimum=20):
    groups = defaultdict(list)
    used = set()
    for number, row in enumerate(rows, 2):
        rid = row["trial_id"].strip()
        sha = row["firmware_sha"].strip()
        image = row["uf2_sha256"].strip()
        trace = row["trace"].strip()
        result = row["success"].strip().upper()
        if not all((rid, sha, image, trace)) or result not in ("PASS", "FAIL"):
            raise ValueError(f"CSV row {number}: missing trial/profile fields or success must be PASS/FAIL")
        key = (sha, image, trace)
        if (key, rid) in used:
            raise ValueError(f"CSV row {number}: duplicate trial_id in the same image")
        used.add((key, rid))
        groups[key].append(result)
    output = []
    for (sha, image, trace), outcomes in sorted(groups.items()):
        passed = outcomes.count("PASS")
        attempts = len(outcomes)
        rate = passed / attempts
        output.append({
            "firmware_sha": sha,
            "uf2_sha256": image,
            "trace": trace,
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
        print(f"{row['firmware_sha']} {row['uf2_sha256']} TRACE={row['trace']}: "
              f"{row['passes']}/{row['attempts']} = {row['success_rate_pct']:.2f}% {status}")
    print("Observed sample rate only; no statistical or physical certification is implied.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

"""Capture diagnostic IMU rows from USB serial, or convert a saved console log."""

import argparse
import csv
import math
from pathlib import Path
import sys
import time


FIELDS = (
    "sequence", "sampled_ms", "raw_x", "raw_y", "raw_z", "pitch_cdeg",
    "left_delta_counts", "right_delta_counts", "calibrated", "state",
    "encoder_valid", "roll_cdeg", "stationary", "height_valid",
    "hump_count", "rise_tenths_mm", "usb_dropped_bytes",
)


def parse_row(line):
    marker = "[IMU_RAW] "
    if marker not in line:
        return None
    values = line.split(marker, 1)[1].strip().split(",")
    if len(values) != len(FIELDS):
        raise ValueError("incomplete IMU row (possible USB loss)")
    return [int(value) for value in values]


def capture(lines, output):
    writer = csv.writer(output)
    writer.writerow(FIELDS)
    rows = bad = gaps = dropped = 0
    previous = None
    for line in lines:
        try:
            row = parse_row(line)
        except ValueError as error:
            bad += 1
            print(str(error), file=sys.stderr)
            continue
        if row is None:
            continue
        if previous is not None and row[0] != ((previous + 1) & 0xFFFFFFFF):
            gaps += 1
        previous = row[0]
        dropped = max(dropped, row[-1])
        writer.writerow(row)
        output.flush()
        rows += 1
    print(f"IMU rows={rows}; sequence gaps/resets={gaps}; "
          f"malformed={bad}; USB dropped bytes={dropped}")
    return 0 if rows and not (bad or gaps or dropped) else 1


def serial_lines(port, seconds):
    try:
        import serial
    except ImportError as error:
        raise RuntimeError("USB capture requires: python -m pip install pyserial") from error
    with serial.Serial(port, baudrate=115200, timeout=0.25) as device:
        deadline = time.monotonic() + seconds
        pending = bytearray()
        while time.monotonic() < deadline:
            pending.extend(device.read_until(b"\n", size=512))
            while b"\n" in pending:
                line, _, pending = pending.partition(b"\n")
                yield line.decode("ascii", errors="replace")
            if len(pending) > 4096:
                raise RuntimeError("serial stream has no line endings")
        if pending:
            yield pending.decode("ascii", errors="replace")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--port", help="USB serial port, e.g. COM5")
    source.add_argument("--input", type=Path, help="saved USB console log")
    parser.add_argument("--seconds", type=float, default=60)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if not math.isfinite(args.seconds) or args.seconds <= 0:
        parser.error("--seconds must be finite and positive")
    try:
        # Exclusive creation preserves previous evidence.
        with args.output.open("x", newline="", encoding="utf-8") as output:
            if args.input:
                with args.input.open(encoding="utf-8", errors="replace") as lines:
                    return capture(lines, output)
            print(f"Capturing {args.port} for {args.seconds:g}s. "
                  "Keep the robot level and still through calibration, then cross the hump.")
            return capture(serial_lines(args.port, args.seconds), output)
    except (OSError, RuntimeError) as error:
        print(str(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())

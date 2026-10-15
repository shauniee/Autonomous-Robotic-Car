"""Capture Pico USB output without sending commands or resetting the board.

Uses the macOS/Linux standard library. Results only pass when a complete
fixture run contains exactly one passing result for every current firmware ID.
"""
import argparse
from datetime import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import select
import termios
import time
from zoneinfo import ZoneInfo


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="Actual Pico USB serial port")
    parser.add_argument("--seconds", type=int, default=60)
    parser.add_argument("--observe", action="store_true", help="Record sensor observations without grading a fixture run")
    parser.add_argument("--image", type=Path, help="Expected flashed UF2; not proof of board identity")
    args = parser.parse_args()
    if not 5 <= args.seconds <= 600:
        parser.error("Use a capture duration from 5 to 600 seconds")
    folder = Path(__file__).resolve().parent
    expected = set()
    for source in folder.glob("*/*.c"):
        expected.update(re.findall(r'TEST_CHECK\s*\(\s*"([A-Z0-9-]+)"', source.read_text()))
    if not expected:
        parser.error("No current firmware fixture definitions found")
    image = None
    if args.image:
        image = {"path": str(args.image), "sha256": hashlib.sha256(args.image.read_bytes()).hexdigest()}
    started = datetime.now(ZoneInfo("Asia/Singapore"))
    stem = started.strftime("%Y-%m-%d_%H%M%S_%f") + "_pico-console"
    output = folder / "integration" / "logs" / stem
    descriptor = os.open(args.port, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
    previous = None
    capture_error = None
    operator_stopped = False
    raw = []
    ending = b""
    try:
        previous = termios.tcgetattr(descriptor)
        settings = termios.tcgetattr(descriptor)
        settings[0] = settings[1] = settings[3] = 0
        settings[2] = termios.CLOCAL | termios.CREAD | termios.CS8
        settings[4] = settings[5] = termios.B115200
        settings[6][termios.VMIN] = settings[6][termios.VTIME] = 0
        termios.tcsetattr(descriptor, termios.TCSANOW, settings)
        print(f"Capturing for {args.seconds}s. " + ("Sensor observation only; no button press is required." if args.observe else "Release, press and release GP20 at the TEST startup prompt."), flush=True)
        deadline = time.monotonic() + args.seconds
        with output.with_suffix(".txt").open("xb") as log:
            while time.monotonic() < deadline:
                if select.select([descriptor], [], [], min(0.2, max(0, deadline-time.monotonic())))[0]:
                    chunk = os.read(descriptor, 8192)
                    if not chunk:
                        break
                    log.write(chunk)
                    log.flush()
                    raw.append(chunk)
                    if not args.observe and b"[TEST] === test run end ===" in ending + chunk:
                        break
                    ending = (ending + chunk)[-64:]
    except KeyboardInterrupt:
        operator_stopped = True
        if not args.observe:
            capture_error = "Interrupted before complete fixture capture"
    except (OSError, termios.error) as error:
        capture_error = f"{type(error).__name__}: {error}"
    finally:
        try:
            if previous is not None:
                termios.tcsetattr(descriptor, termios.TCSANOW, previous)
        except (OSError, termios.error) as error:
            capture_error = capture_error or f"Restore failed: {error}"
        finally:
            os.close(descriptor)
    text = b"".join(raw).decode("utf-8", errors="replace")
    results = re.findall(r"\[TEST\]\s+([A-Z0-9-]+)\s+(PASS|FAIL)\b", text)
    seen = {}
    for case, result in results:
        seen.setdefault(case, []).append(result)
    failed = sorted(case for case, values in seen.items() if "FAIL" in values)
    missing = sorted(expected-set(seen))
    unexpected = sorted(set(seen)-expected)
    duplicate = sorted(case for case, values in seen.items() if len(values) != 1)
    complete = "[TEST] === test run start ===" in text and "[TEST] === test run end ===" in text
    passed = complete and not (failed or missing or unexpected or duplicate or capture_error)
    summary = {
        "started": started.isoformat(), "port": args.port,
        "method": "Passive USB serial capture; no commands, flash or motor requests sent",
        "expected_image_operator_supplied": image,
        "image_identity_verified_on_device": False,
        "log": output.with_suffix(".txt").name if output.with_suffix(".txt").exists() else None,
        "bytes": len(b"".join(raw)), "expected_ids": sorted(expected),
        "complete_run_observed": complete, "observed_results": seen,
        "failed": failed, "missing": missing, "unexpected": unexpected,
        "duplicate": duplicate, "firmware_result": "NOT_EVALUATED" if args.observe else "PASS" if passed else "INCOMPLETE_OR_FAIL",
        "sensor_observation_only": args.observe,
        "capture_error": capture_error,
        "operator_stopped": operator_stopped,
        "physical_mission_acceptance_claimed": False,
    }
    with output.with_suffix(".json").open("x") as report:
        json.dump(summary, report, indent=2)
        report.write("\n")
    print(json.dumps({"result": summary["firmware_result"], "log": str(output.with_suffix(".txt")), "summary": str(output.with_suffix(".json"))}))
    return 0 if (args.observe and raw and not capture_error) or passed else 1


if __name__ == "__main__":
    raise SystemExit(main())

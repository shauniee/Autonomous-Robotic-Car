#!/usr/bin/env python3
"""Check that each subsystem still delivers what the project owes.

Fails when a deliverable's source, public function, wired-in test or report
section is missing, or when subsystems stop agreeing with each other. A pass
means the artefact exists and its tests run. It never means the robot met the
requirement: anything that needs the car prints NEEDS EVIDENCE, as a build or
host test is not a physical pass.

    check_deliverables.py          check this repository
    check_deliverables.py ROOT     check another tree
"""
import re
import sys
from pathlib import Path

COMMS = "app_program/comms"
MOTION = "app_program/motion"
LINE = "app_program/line_barcode"
TERRAIN = "app_program/terrain"
OBSTACLE = "app_program/obstacle"
MISSION = "app_program/mission"

# name, source files, public functions, tests, (document, section), evidence
DELIVERABLES = {
    "comms": [
        {"name": "Communication API",
         "files": [f"{COMMS}/comms.h", f"{COMMS}/comms.c"],
         "functions": [(f"{COMMS}/comms.h", "comms_event")],
         "tests": ["test_mqtt_frame.c"]},
        {"name": "Telemetry framework",
         "files": [f"{COMMS}/report.c", f"{COMMS}/mqtt_frame.c"],
         "functions": [(f"{COMMS}/comms.h", "mqtt_encode_telemetry"),
                       (f"{COMMS}/comms.h", "mqtt_encode_terrain_trace")],
         "tests": ["test_comms_report.c", "test_comms_stress.c"]},
        {"name": "MQTT documentation",
         "sections": [("team.md", "MQTT binary protocol")]},
        {"name": "Telemetry demonstration",
         "files": ["scripts/mqtt_client.py", "scripts/dashboard.html"],
         "tests": ["test_mqtt_cli.py", "test_dashboard.mjs"],
         "evidence": "telemetry demonstration recorded"},
    ],
    "motion": [
        {"name": "Motion-control library",
         "files": [f"{MOTION}/motion.h", f"{MOTION}/motion_control.c"],
         "functions": [(f"{MOTION}/motion.h", "motion_command"),
                       (f"{MOTION}/motion.h", "motion_stop"),
                       (f"{MOTION}/motion.h", "motion_stop_distance_mm")],
         "tests": ["test_motion_control.c"]},
        {"name": "PID tuning report",
         "sections": [(f"{MOTION}/README.md", "PID tuning report")],
         "evidence": "measured step responses on the floor"},
        {"name": "Motion accuracy evaluation",
         "sections": [(f"{MOTION}/README.md", "Motion accuracy evaluation")],
         "evidence": "taped distance and turn-angle errors"},
    ],
    "line_barcode": [
        {"name": "Line-following module",
         "files": [f"{LINE}/line.c"],
         "functions": [(f"{LINE}/line_barcode.h", "ir_tracking_sample")],
         "tests": ["test_ir_tracking.c", "test_line_replay.c"]},
        {"name": "Barcode decoder",
         "files": [f"{LINE}/barcode.c"],
         "functions": [(f"{LINE}/line_barcode.h", "barcode_pop")],
         "tests": ["test_barcode.c"],
         "evidence": "all letters and a flipped print on the car"},
        {"name": "Navigation command interface",
         "files": [f"{MISSION}/mission_mapping.h"],
         "functions": [(f"{MISSION}/mission_mapping.h",
                        "mission_mapping_submit"),
                       (f"{MISSION}/mission_mapping.h",
                        "mission_direction_submit")],
         "tests": ["test_mission_mapping.c", "test_smooth_navigation.c"]},
    ],
    "terrain": [
        {"name": "IMU processing module",
         "files": [f"{TERRAIN}/terrain_algo.c", f"{TERRAIN}/terrain.c"],
         "functions": [(f"{TERRAIN}/terrain.h", "terrain_algo_process")],
         "tests": ["test_terrain.c"]},
        {"name": "Hump-detection algorithm",
         "files": [f"{TERRAIN}/terrain_algo.c"],
         "functions": [(f"{TERRAIN}/terrain.h", "terrain_height_mm")],
         "tests": ["test_terrain.c"],
         "evidence": "exact hump count and height on measured humps"},
        {"name": "Terrain analysis report",
         "sections": [(f"{TERRAIN}/README.md", "Terrain analysis")],
         "evidence": "measured reference humps"},
    ],
    "obstacle": [
        {"name": "Scanning subsystem",
         "files": [f"{OBSTACLE}/obstacle.c"],
         "functions": [(f"{OBSTACLE}/obstacle.h", "obstacle_scan_request")],
         "tests": ["test_echo_range.c", "test_range_filter.c"]},
        {"name": "Obstacle profile generator",
         "files": [f"{OBSTACLE}/obstacle_profile.c"],
         "functions": [(f"{OBSTACLE}/obstacle.h",
                        "obstacle_profile_estimate")],
         "tests": ["test_obstacle_profile.c", "test_profile_edges.c"]},
        {"name": "Avoidance and recovery algorithms",
         "files": [f"{MISSION}/bypass_control.c",
                   f"{MISSION}/recovery_control.c"],
         "functions": [(f"{MISSION}/bypass_control.h", "bypass_control_begin")],
         "tests": ["test_mission.c"],
         "evidence": "both sides and a blocked side, with gaps measured"},
    ],
}

# Calls only Motion may make; Motion owns the motors.
MOTOR_WRITE = re.compile(r"\bdrv_motor_\w+\s*\(")
# Calls that would let networking start, stop or drive the robot.
DRIVE_CALL = re.compile(
    r"\bmotion_(?:command|stop|reset_stop|inhibit_startup)\s*\(")


def read(root, relative):
    """Return a file's text, or None if it does not exist."""
    path = root / relative
    return path.read_text(errors="replace") if path.is_file() else None


def check_deliverable(root, item, test_runner):
    """Return the problems with one deliverable."""
    problems = []
    for relative in item.get("files", []):
        if read(root, relative) is None:
            problems.append(f"missing file {relative}")
    for relative, function in item.get("functions", []):
        text = read(root, relative)
        if text is None or not re.search(rf"\b{function}\s*\(", text):
            problems.append(f"{relative} does not declare {function}()")
    for test in item.get("tests", []):
        # Host tests live in their test ID folder: testing/<sub>/logs/<ID>/.
        if not any((root / "testing").rglob(test)):
            problems.append(f"missing test {test}")
        elif test not in test_runner:
            problems.append(f"{test} is not run by CI")
    for relative, section in item.get("sections", []):
        text = read(root, relative)
        pattern = rf"^#+ .*{re.escape(section)}"
        if text is None or not re.search(pattern, text, re.M):
            problems.append(f"{relative} has no section '{section}'")
    return problems


def subsystem_sources(root, name):
    """Yield (relative path, text) for one app_program folder's C code."""
    folder = root / "app_program" / name
    for path in sorted(folder.glob("*.[ch]")) if folder.is_dir() else []:
        yield path.relative_to(root).as_posix(), path.read_text(errors="replace")


def check_alignment(root):
    """Return places where subsystems no longer agree with each other."""
    problems = []
    names = sorted(p.name for p in (root / "app_program").iterdir()
                   if p.is_dir() and p.name != "common")
    for name in names:
        for relative, text in subsystem_sources(root, name):
            if name != "motion" and MOTOR_WRITE.search(text):
                problems.append(f"{relative} writes the motors; only Motion "
                                "may")
    for relative, text in subsystem_sources(root, "comms"):
        if DRIVE_CALL.search(text):
            problems.append(f"{relative} commands Motion; networking never "
                            "starts, stops or drives the robot")
    for name in DELIVERABLES:
        readme = read(root, f"app_program/{name}/README.md")
        if readme is None:
            problems.append(f"{name} has no README.md")
        elif "## Contents" not in readme:
            problems.append(f"{name}/README.md has no contents list")
    problems += check_wire_lengths(root)
    return problems


def check_wire_lengths(root):
    """The firmware and the laptop tool must agree on every frame length."""
    header = read(root, f"{COMMS}/comms.h")
    client = read(root, "scripts/mqtt_client.py")
    if header is None or client is None:
        return ["comms.h or mqtt_client.py is missing"]
    firmware = dict(re.findall(r"#define MQTT_LEN_(\w+)\s+(\d+)u", header))
    table = re.search(r"^LENGTHS = \{(.*?)\}", client, re.M | re.S)
    laptop = dict(re.findall(r"(\w+): (\d+)", table.group(1))) if table else {}
    return [f"frame {name}: firmware {length} bytes, mqtt_client.py "
            f"{laptop[name]}" for name, length in firmware.items()
            if name in laptop and laptop[name] != length]


def main(argv):
    root = Path(argv[0] if argv else Path(__file__).resolve().parents[1])
    test_runner = (read(root, "testing/tools/run.sh") or "") + \
        (read(root, ".github/workflows/host-tests.yml") or "")
    failed = 0
    awaiting = 0
    for subsystem, items in DELIVERABLES.items():
        print(subsystem)
        for item in items:
            problems = check_deliverable(root, item, test_runner)
            failed += len(problems)
            state = "MISSING" if problems else "OK"
            print(f"  {state:8} {item['name']}")
            for problem in problems:
                print(f"           {problem}")
            if "evidence" in item:
                awaiting += 1
                print(f"           NEEDS EVIDENCE: {item['evidence']}")
    print("alignment")
    problems = check_alignment(root)
    failed += len(problems)
    for problem in problems or ["subsystems agree"]:
        print(f"  {'MISSING' if problems else 'OK':8} {problem}")
    print(f"{failed} problem(s); {awaiting} deliverable(s) still need evidence "
          "from the car, which CI cannot supply")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

"""Laptop side of the robot's binary MQTT protocol (schema v2). Python 3.10+.

  monitor          print telemetry, events and acknowledgements as readable text
  map L R S U      send the four mission-day letters (left, right, straight, U-turn)
  snapshot         request one telemetry snapshot
  --self-test      decode the golden vectors shared with testing/pure/test_mqtt_frame.c

Frame layouts: docs/architecture.md (MQTT binary protocol). Speed and
direction are derived here from encoder counts between telemetry frames, so
the robot does not spend bits on them.
"""
import argparse
import os
import re
import struct
import sys
import threading
import time
from pathlib import Path

VERSION = 2
TELEMETRY, EVENT, SNAPSHOT_REPLY, MAPPING, ACK, SNAPSHOT_REQUEST = 1, 2, 3, 4, 5, 6
LENGTHS = {TELEMETRY: 23, EVENT: 11, SNAPSHOT_REPLY: 23, MAPPING: 5, ACK: 3, SNAPSHOT_REQUEST: 2}

MISSION = ["CALIBRATE", "READY", "FOLLOW", "JUNCTION_TURN", "BYPASS", "COMPLETE", "STOP"]
MOTION = ["IDLE", "RUNNING", "DONE", "FAILED"]
LINE = ["UNKNOWN", "VISIBLE", "LOST", "BROAD_BLACK"]
ACTION = ["UNMAPPED", "LEFT", "RIGHT", "STRAIGHT", "U_TURN"]
EVENT_KIND = {1: "BARCODE", 2: "HUMP", 3: "OBSTACLE", 4: "TURN"}
ACK_RESULT = ["OK", "BUSY (robot moving)", "INVALID letters", "MALFORMED"]
# One byte per owner, rd_owner_t order; bit names follow app_program/common/robot_data.h.
FAULTS = [
    ("Motion", ["motor stall", "encoder"]),
    ("Line", ["IR invalid", "barcode capture overflow", "barcode event overflow"]),
    ("Terrain", ["IMU", "IMU calibration failed", "impact"]),
    ("Obstacle", ["no echo"]),
    ("Mission", ["line lost"]),
    ("Network", ["WiFi lost", "broker lost"]),
]

GOLDEN_TELEMETRY = bytes([0x21, 0x2A, 0x72, 0x45, 0x01, 0x02, 0x04, 0x08, 0x01, 0x03, 0x34, 0x12, 0x78, 0x56,
                          0x03, 0xF8, 0xD0, 0x30, 0x61, 0xA4, 0x01, 0xAA, 0x00])
GOLDEN_EVENT = bytes([0x22, 0x01, 0x19, 0xE8, 0x03, 0x10, 0x00, 0x11, 0x00, 0x02, 0x00])
GOLDEN_MAPPING = bytes([0x24, 0x07, 0x20, 0x88, 0x01])


def config_value(name, default):
    """Reads a numeric #define from robot_config.h so the scale has one owner."""
    path = Path(__file__).resolve().parent.parent / "app_program/common/robot_config.h"
    match = re.search(rf"#define\s+{name}\s+([0-9.]+)", path.read_text()) if path.exists() else None
    return float(match.group(1)) if match else default


COUNTS_PER_MM = config_value("ENC_COUNTS_PER_REV", 360.0) / (3.14159265 * config_value("WHEEL_DIAMETER_MM", 65.0))
HUMP_MM_PER_CODE = config_value("HUMP_MM_PER_COUNT", 1.0)


def name(table, index):
    return table[index] if index < len(table) else f"?{index}"


def letter(code):
    return chr(ord("A") + code) if code < 26 else "-"


def frame_type(frame):
    if not frame or frame[0] >> 4 != VERSION:
        raise ValueError(f"not schema v{VERSION}")
    kind = frame[0] & 0xF
    if LENGTHS.get(kind) != len(frame):
        raise ValueError(f"type {kind} with length {len(frame)}")
    return kind


def decode_telemetry(f):
    left, right = struct.unpack_from("<HH", f, 10)
    (front,) = struct.unpack_from("<H", f, 16)
    width, depth = struct.unpack_from("<HH", f, 19)
    return {
        "sequence": f[1], "mission": name(MISSION, f[2] & 0xF), "motion": name(MOTION, (f[2] >> 4) & 3),
        "line": LINE[f[2] >> 6], "action_id": f[3] & 0x3F, "mapped": bool(f[3] & 0x40),
        "faults": list(f[4:10]), "left": left, "right": right, "hump_count": f[14],
        "hump_max_mm": (f[15] & 0x7F) * HUMP_MM_PER_CODE if f[15] & 0x80 else None,
        "front_mm": front & 0x1FFF if front & 0x2000 else None,
        "barcode": letter(f[18] & 0x1F), "barcode_action": name(ACTION, f[18] >> 5),
        "obstacle_width_mm": width, "obstacle_depth_mm": depth,
    }


def decode_event(f):
    kind, arg, time_ms, left, right, arg2 = f[1] & 0xF, f[2], *struct.unpack_from("<HHHH", f, 3)
    detail = {1: f"letter {letter(arg)} -> {name(ACTION, arg2)}", 2: f"height {arg * HUMP_MM_PER_CODE:g} mm",
              3: f"depth {arg2} mm", 4: f"turn {name(ACTION, arg)}"}.get(kind, f"arg {arg} arg2 {arg2}")
    return f"EVENT {EVENT_KIND.get(kind, kind)}: {detail} at t={time_ms} ms, counts L={left} R={right}"


def header_fault_widths(text=None):
    """Bits per owner byte in robot_data.h; each owner's bits restart at 1u << 0."""
    path = Path(__file__).resolve().parent.parent / "app_program/common/robot_data.h"
    widths = []
    for bit in map(int, re.findall(r"RD_FAULT_\w+\s*=\s*1u\s*<<\s*(\d+)", text or path.read_text())):
        if bit == 0:
            widths.append(0)
        widths[-1] = max(widths[-1], bit + 1)
    return widths


def fault_text(fault_bytes):
    active = [f"{owner}: {bit}" for (owner, bits), byte in zip(FAULTS, fault_bytes)
              for i, bit in enumerate(bits) if byte >> i & 1]
    return ", ".join(active) or "none"


class Motion:
    """Speed (mm/s) and direction from successive encoder counts."""

    def __init__(self):
        self.last = None

    def update(self, left, right):
        now = time.monotonic()
        previous, self.last = self.last, (now, left, right)
        if previous is None or now - previous[0] <= 0:
            return "speed n/a"
        dt = now - previous[0]
        speeds = [((count - old + 0x8000) & 0xFFFF) - 0x8000 for count, old in ((left, previous[1]), (right, previous[2]))]
        vl, vr = (d / COUNTS_PER_MM / dt for d in speeds)
        if abs(vl) < 5 and abs(vr) < 5:
            direction = "stopped"
        elif abs(vl - vr) < 20:
            direction = "straight" if vl + vr > 0 else "reverse"
        else:
            direction = "left" if vr > vl else "right"
        return f"speed L={vl:.0f} R={vr:.0f} mm/s ({direction})"


def describe(frame, motion):
    kind = frame_type(frame)
    if kind in (TELEMETRY, SNAPSHOT_REPLY):
        t = decode_telemetry(frame)
        label = "SNAPSHOT" if kind == SNAPSHOT_REPLY else "TELEMETRY"
        hump = f"{t['hump_max_mm']:g} mm" if t["hump_max_mm"] is not None else "none"
        front = f"{t['front_mm']} mm" if t["front_mm"] is not None else "no echo"
        return (f"{label} #{t['sequence']} state={t['mission']} motion={t['motion']}(id {t['action_id']}) "
                f"line={t['line']} mapped={'yes' if t['mapped'] else 'NO'} | counts L={t['left']} R={t['right']} "
                f"{motion.update(t['left'], t['right'])} | humps={t['hump_count']} max={hump} | front={front} | "
                f"barcode={t['barcode']}->{t['barcode_action']} | obstacle width={t['obstacle_width_mm']} mm "
                f"depth={t['obstacle_depth_mm']} mm | faults: {fault_text(t['faults'])}")
    if kind == EVENT:
        return decode_event(frame)
    if kind == ACK:
        return f"MAPPING ACK seq {frame[1]}: {name(ACK_RESULT, frame[2])}"
    return f"frame type {kind}"


def mapping_frame(sequence, letters):
    codes = [ord(c) - ord("A") for c in letters]
    packed = sum(code << (5 * i) for i, code in enumerate(codes))
    return bytes([VERSION << 4 | MAPPING, sequence]) + packed.to_bytes(3, "little")


def self_test():
    t = decode_telemetry(GOLDEN_TELEMETRY)
    assert frame_type(GOLDEN_TELEMETRY) == TELEMETRY and t["sequence"] == 0x2A and t["mission"] == "FOLLOW"
    assert t["motion"] == "FAILED" and t["line"] == "VISIBLE" and t["action_id"] == 5 and t["mapped"]
    assert t["faults"] == [1, 2, 4, 8, 1, 3] and (t["left"], t["right"]) == (0x1234, 0x5678)
    assert t["hump_count"] == 3 and t["front_mm"] == 0x10D0 and t["barcode"] == "B"
    assert t["barcode_action"] == "STRAIGHT" and (t["obstacle_width_mm"], t["obstacle_depth_mm"]) == (420, 170)
    assert "BARCODE: letter Z -> RIGHT" in decode_event(GOLDEN_EVENT)
    assert mapping_frame(7, "ABCD") == GOLDEN_MAPPING
    assert [len(bits) for _, bits in FAULTS] == header_fault_widths(), "FAULTS disagrees with robot_data.h"
    print("mqtt_client self-test: PASS")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("action", nargs="?", choices=("monitor", "map", "snapshot"))
    parser.add_argument("letters", nargs="*", help="map: four distinct letters for left, right, straight, U-turn")
    parser.add_argument("--broker", help="broker address")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--root", default="robot/s3", help="topic root (firmware: <root>/telemetry, /request, /status)")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if not args.action or not args.broker:
        parser.error("an action and --broker are required")
    letters = "".join(args.letters).upper()
    if args.action == "map" and (len(letters) != 4 or len(set(letters)) != 4 or not letters.isalpha() or not letters.isascii()):
        parser.error("map needs four distinct letters A-Z, e.g. map A B C D")
    try:
        import paho.mqtt.client as mqtt
    except ImportError:
        parser.exit(1, "Install: python -m pip install -r scripts/requirements-mqtt.txt\n")

    ready, done = threading.Event(), threading.Event()
    sequence = int(time.time()) & 0xFF
    motion = Motion()
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    if os.getenv("ROBOT_MQTT_USERNAME"):
        client.username_pw_set(os.getenv("ROBOT_MQTT_USERNAME"), os.getenv("ROBOT_MQTT_PASSWORD", ""))

    def on_connect(client, userdata, flags, reason_code, properties):
        if reason_code.is_failure:
            print(f"Broker refused connection: {reason_code}")
            done.set()
            return
        client.subscribe([(args.root + "/telemetry", 0), (args.root + "/status", 1)])
        ready.set()

    def on_message(client, userdata, message):
        if message.topic.endswith("/status"):
            print(f"STATUS {message.payload.decode('utf-8', 'replace')}", flush=True)
            return
        try:
            line = describe(bytes(message.payload), motion)
        except ValueError as error:
            print(f"undecodable frame ({error}): {message.payload.hex()}", flush=True)
            return
        if args.action == "monitor":
            print(line, flush=True)
        elif args.action == "map" and line.startswith(f"MAPPING ACK seq {sequence}:"):
            print(line, flush=True)
            done.set()
        elif args.action == "snapshot" and line.startswith("SNAPSHOT"):
            print(line, flush=True)
            done.set()

    client.on_connect = on_connect
    client.on_message = on_message
    try:
        client.connect(args.broker, args.port, keepalive=15)
        client.loop_start()
        if not ready.wait(8):
            raise RuntimeError("timed out connecting to the broker")
        if args.action == "monitor":
            print("Monitoring; Ctrl+C to exit.", flush=True)
            done.wait()
            return 0
        payload = mapping_frame(sequence, letters) if args.action == "map" else bytes([VERSION << 4 | SNAPSHOT_REQUEST, sequence])
        client.publish(args.root + "/request", payload, qos=1).wait_for_publish(timeout=5)
        if not done.wait(8):
            raise RuntimeError("no reply from the robot (is it online and stopped?)")
        return 0
    except KeyboardInterrupt:
        return 0
    except (OSError, RuntimeError) as error:
        print(f"Error: {error}")
        return 1
    finally:
        client.disconnect()
        client.loop_stop()


if __name__ == "__main__":
    sys.exit(main())

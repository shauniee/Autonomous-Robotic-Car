"""Laptop side of the robot's binary MQTT protocol (schema v2). Python 3.10+.

  monitor          print telemetry, events and WHEEL/LINE traces, folding exact repeats (--raw: every frame as JSON)
  map L R S U      send the four mission-day letters (left, right, straight, U-turn)
  snapshot         request one telemetry snapshot
  serve            live web dashboard at http://127.0.0.1:8080 (scripts/dashboard.html);
                   the dashboard can send mappings and next-junction test directions
  --self-test      decode the golden vectors shared with testing/host/test_mqtt_frame.c

Frame layouts: docs/architecture.md (MQTT binary protocol). Speed and
direction are derived here from encoder counts between telemetry frames, so
normal telemetry stays compact. Separate WHEEL traces report Motion-release targets,
measured speeds and serviced duty for PID diagnosis; LINE traces report each IR
acquisition Mission acted on (sensor bits, junction flags, steering, command);
network gaps are possible.
"""
import argparse
import json
import queue
import os
import re
import struct
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

VERSION = 3
TELEMETRY, EVENT, SNAPSHOT_REPLY, MAPPING, ACK, SNAPSHOT_REQUEST, JUNCTION_TEST = 1, 2, 3, 4, 5, 6, 7
WHEEL_TRACE, LINE_TRACE, BARCODE_TRACE, TERRAIN_TRACE = 8, 9, 10, 11
LENGTHS = {TELEMETRY: 18, EVENT: 11, SNAPSHOT_REPLY: 18, MAPPING: 5, ACK: 3, SNAPSHOT_REQUEST: 2, JUNCTION_TEST: 3, WHEEL_TRACE: 15, LINE_TRACE: 17, BARCODE_TRACE: 18, TERRAIN_TRACE: 14}

MISSION = ["READY", "FOLLOW_LINE", "NAVIGATE", "SCAN", "BYPASS", "REACQUIRE", "STOPPED"]
MOTION = ["IDLE", "RUNNING", "DONE", "FAILED"]
LINE = ["UNKNOWN", "VISIBLE", "LOST", "BROAD_BLACK"]
ACTION = ["UNMAPPED", "LEFT", "RIGHT", "STRAIGHT", "U_TURN"]
EVENT_KIND = {1: "BARCODE", 2: "HUMP", 3: "OBSTACLE", 4: "TURN"}
ACK_RESULT = ["OK", "BUSY", "INVALID request", "MALFORMED"]
# Owners in rd_owner_t order; bit names follow app_program/common/robot_data.h.
# Telemetry packs them in this order into one 12-bit word, bits 12-15 reserved.
FAULTS = [
    ("Motion", ["motor stall", "encoder"]),
    ("Line", ["IR invalid", "barcode capture overflow", "barcode event overflow"]),
    ("Terrain", ["IMU", "IMU calibration failed", "lifted"]),
    ("Obstacle", ["no echo"]),
    ("Mission", ["line lost"]),
    ("Network", ["WiFi lost", "broker lost"]),
]

GOLDEN_TELEMETRY = bytes([0x31, 0x2A, 0x72, 0x45, 0x89, 0x0F, 0x34, 0x12, 0x78, 0x56,
                          0x03, 0xF8, 0xD0, 0x30, 0x61, 0xA4, 0xA9, 0x02])
GOLDEN_EVENT = bytes([0x32, 0x01, 0x19, 0xE8, 0x03, 0x10, 0x00, 0x11, 0x00, 0x02, 0x00])
GOLDEN_MAPPING = bytes([0x34, 0x07, 0x20, 0x88, 0x01])


def config_value(name, default):
    """Reads a numeric #define from robot_config.h so the scale has one owner."""
    path = Path(__file__).resolve().parent.parent / "app_program/common/robot_config.h"
    match = re.search(rf"#define\s+{name}\s+([0-9.]+)", path.read_text()) if path.exists() else None
    return float(match.group(1)) if match else default


COUNTS_PER_MM = config_value("ENC_COUNTS_PER_MM", 11.649)
HUMP_MM_PER_CODE = config_value("HUMP_MM_PER_COUNT", 1.0)
TRACK_WIDTH_MM = config_value("TRACK_WIDTH_MM", 120.0)


def turn_rate_dps(vl, vr):
    """FR-20 turn rate in deg/s from wheel speeds in mm/s; positive turns left, as enc_heading_deg."""
    return (vr - vl) / TRACK_WIDTH_MM * 57.2957795


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


def unpack_faults(word):
    """The 12-bit fault word as one value per owner, rd_owner_t order."""
    faults, shift = [], 0
    for _, bits in FAULTS:
        faults.append(word >> shift & ((1 << len(bits)) - 1))
        shift += len(bits)
    return faults


def decode_telemetry(f):
    if frame_type(f) not in (TELEMETRY, SNAPSHOT_REPLY):
        raise ValueError("not a telemetry frame")
    fault_word, left, right = struct.unpack_from("<HHH", f, 4)
    (front,) = struct.unpack_from("<H", f, 12)
    if f[3] & 0x80 or fault_word >> 12:
        raise ValueError("telemetry reserved bits set")
    calibration = front >> 14
    if calibration == 3:
        raise ValueError("invalid IMU calibration status")
    code = f[14] & 0x1F
    if (code > 25 and code != 31) or f[14] >> 5 > 4:
        raise ValueError("invalid barcode letter/action")
    if front & 0x2000 and front & 0x1FFF == 0x1FFF:
        raise ValueError("invalid range marked valid")
    if f[11] == 0xFF:
        raise ValueError("invalid hump height marked valid")
    size = int.from_bytes(f[15:18], "little")
    if size >> 20:
        raise ValueError("reserved telemetry bits set")
    width, clearance = size & 0x3FF, size >> 10 & 0x3FF
    return {
        "sequence": f[1], "mission": name(MISSION, f[2] & 0xF), "motion": name(MOTION, (f[2] >> 4) & 3),
        "line": LINE[f[2] >> 6], "action_id": f[3] & 0x3F, "mapped": bool(f[3] & 0x40),
        "faults": unpack_faults(fault_word), "left": left, "right": right, "hump_count": f[10],
        "hump_max_mm": (f[11] & 0x7F) * HUMP_MM_PER_CODE if f[11] & 0x80 else None,
        "imu_calibrated": None if calibration == 0 else calibration == 2,
        "front_mm": front & 0x1FFF if front & 0x2000 else None,
        "barcode": letter(f[14] & 0x1F), "barcode_action": name(ACTION, f[14] >> 5),
        "obstacle_width_mm": width, "obstacle_clearance_mm": clearance,
    }


def decode_wheel_trace(f):
    if frame_type(f) != WHEEL_TRACE:
        raise ValueError("not a wheel trace")
    sampled_ms, targets, vl, vr, dl, dr = struct.unpack_from("<IIhhbb", f, 1)
    if any(d != -128 and not -100 <= d <= 100 for d in (dl, dr)):
        raise ValueError("invalid trace duty")
    output_ok = bool(targets & (1 << 31))
    if not output_ok and (dl, dr) != (-128, -128):
        raise ValueError("failed output marked with valid duty")
    def speed(value, bits):
        value = value - (1 << bits) if value & (1 << (bits - 1)) else value
        return None if value == -(1 << (bits - 1)) else round(value / COUNTS_PER_MM, 2)
    return dict(type="wheel_trace", sampled_ms=sampled_ms,
                target_left=speed(targets & 0x7FFF, 15),
                target_right=speed((targets >> 15) & 0x7FFF, 15),
                measured_left=speed(vl & 0xFFFF, 16), measured_right=speed(vr & 0xFFFF, 16),
                duty_left=None if dl == -128 else dl, duty_right=None if dr == -128 else dr,
                inhibited=bool(targets & (1 << 30)), output_ok=output_ok)


LINE_FLAGS = ("left_black", "right_black", "in_broad", "junction_open",
              "junction_qualified", "entry_curved", "junction_locked", "barcode_level")
COMMAND_MODE = ["STOP", "FOLLOW", "DISTANCE", "TURN"]


def decode_line_trace(f):
    if frame_type(f) != LINE_TRACE:
        raise ValueError("not a line trace")
    sampled_ms, flags, state, left, right, steer, law, command_steer, command, skew = \
        struct.unpack_from("<IBBHHbbbHB", f, 1)
    if state & 0x80 or (state & 0xF) >= len(MISSION):
        raise ValueError("invalid line trace")
    trace = dict(type="line_trace", sampled_ms=sampled_ms,
                 mission=MISSION[state & 0xF], phase=state >> 4, left=left, right=right,
                 steer_q=steer, law_q=law, command=COMMAND_MODE[command >> 9 & 3], barcode_edges=command >> 11,
                 command_steer_q=command_steer, command_speed=command & 0x1FF,
                 onset_skew_mm=None if skew == 255 else skew)
    trace.update({key: bool(flags >> bit & 1) for bit, key in enumerate(LINE_FLAGS)})
    return trace


def decode_terrain_trace(f):
    """IMU sample: tilt, axes, height, peak rise, travel, state and validity flags."""
    if frame_type(f) != TERRAIN_TRACE:
        raise ValueError("not a terrain trace")
    time_ms, pitch, flags, rise, accel, roll, x, y, z, height, travel = struct.unpack_from("<HbBBBbbbbBH", f, 1)
    return {"type": "terrain_trace", "time_ms": time_ms, "pitch_deg": pitch / 4, "roll_deg": roll / 4,
            "state": flags & 7, "rise_mm": rise, "accel_mg": accel * 10, "x_mg": x * 16, "y_mg": y * 16,
            "z_mg": z * 16, "height_mm": height, "travel_mm": travel,
            "encoder_ok": bool(flags >> 6 & 1), "calibrated": bool(flags >> 7)}


def decode_barcode_trace(f):
    """Sequence, then raw GP27 runs: H black or L white, width in encoder counts."""
    if frame_type(f) != BARCODE_TRACE:
        raise ValueError("not a barcode trace")
    runs = struct.unpack_from("<8H", f, 2)
    return f"#{f[1]} " + " ".join(f"{'H' if r >> 15 else 'L'}{r & 0x7FFF}" for r in runs if r & 0x7FFF)


def decode_event(f):
    if frame_type(f) != EVENT:
        raise ValueError("not an event frame")
    if f[1] & 0xF0 or f[1] not in EVENT_KIND:
        raise ValueError("invalid event kind/reserved bits")
    kind, arg, time_ms, left, right, arg2 = f[1] & 0xF, f[2], *struct.unpack_from("<HHHH", f, 3)
    detail = {1: f"letter {letter(arg)} -> {name(ACTION, arg2)}", 2: f"height {arg * HUMP_MM_PER_CODE:g} mm",
              3: f"passing {'left' if arg else 'right'}, clearance {arg2} mm", 4: f"turn {name(ACTION, arg)}"}.get(kind, f"arg {arg} arg2 {arg2}")
    return f"EVENT {EVENT_KIND.get(kind, kind)}: {detail} at t={time_ms} ms, counts L={left} R={right}"


def header_fault_widths():
    """Bits per owner byte in robot_data.h; each owner's bits restart at 1u << 0.
    Reads the source tree, so the self-test runs only inside the repository."""
    path = Path(__file__).resolve().parent.parent / "app_program/common/robot_data.h"
    bits = [int(b) for b in re.findall(r"RD_FAULT_\w+\s*=\s*1u\s*<<\s*(\d+)", path.read_text())]
    assert bits and bits[0] == 0, "robot_data.h fault enum must start each owner at bit 0"
    widths = []
    for bit in bits:
        if bit == 0:
            widths.append(0)
        widths[-1] = max(widths[-1], bit + 1)
    return widths


def active_faults(fault_bytes):
    return [f"{owner}: {bit}" for (owner, bits), byte in zip(FAULTS, fault_bytes)
            for i, bit in enumerate(bits) if byte >> i & 1]


def fault_text(fault_bytes):
    return ", ".join(active_faults(fault_bytes)) or "none"


class Motion:
    """Speed (mm/s) and direction from successive encoder counts."""

    def __init__(self):
        self.last = None

    def measure(self, left, right):
        """Arrival-time speed estimate, not the motor's measured encoder speed.

        Telemetry is emitted about every 500 ms, but MQTT delivery is not a
        periodic clock. Back-to-back delivery can make a 500 ms wheel count
        change look like a huge speed spike (2026-11-02). Report unavailable when
        the arrival gap is implausible instead of displaying false precision.
        For actual per-wheel tuning use the timestamped WHEEL diagnostic frames.
        """
        now = time.monotonic()
        previous, self.last = self.last, (now, left, right)
        if previous is None:
            return None
        dt = now - previous[0]
        if not (0.25 <= dt <= 2.0):
            return None
        speeds = [((count - old + 0x8000) & 0xFFFF) - 0x8000 for count, old in ((left, previous[1]), (right, previous[2]))]
        vl, vr = (d / COUNTS_PER_MM / dt for d in speeds)
        if abs(vl) < 5 and abs(vr) < 5:
            direction = "stopped"
        elif abs(vl - vr) < 20:
            direction = "straight" if vl + vr > 0 else "reverse"
        else:
            direction = "left" if vr > vl else "right"
        return vl, vr, direction

    def update(self, left, right):
        measured = self.measure(left, right)
        if measured is None:
            return "speed n/a"
        vl, vr, direction = measured
        return f"speed L={vl:.0f} R={vr:.0f} mm/s ({direction}) turn={turn_rate_dps(vl, vr):.0f} deg/s"


def describe(frame, motion):
    kind = frame_type(frame)
    if kind == WHEEL_TRACE:
        return "WHEEL " + json.dumps(decode_wheel_trace(frame), separators=(",", ":"))
    if kind == TERRAIN_TRACE:
        t = decode_terrain_trace(frame)
        return (f"HUMP t={t['time_ms']} pitch={t['pitch_deg']:g} roll={t['roll_deg']:g} state={t['state']} "
                f"rise={t['rise_mm']}mm accel={t['accel_mg']}mg x={t['x_mg']} y={t['y_mg']} z={t['z_mg']} "
                f"height={t['height_mm']}mm travel={t['travel_mm']}mm encoder_ok={t['encoder_ok']:d} "
                f"calibrated={t['calibrated']:d}")
    if kind == BARCODE_TRACE:
        return "BARS " + decode_barcode_trace(frame)
    if kind == LINE_TRACE:
        return "LINE " + json.dumps(decode_line_trace(frame), separators=(",", ":"))
    if kind in (TELEMETRY, SNAPSHOT_REPLY):
        t = decode_telemetry(frame)
        label = "SNAPSHOT" if kind == SNAPSHOT_REPLY else "TELEMETRY"
        hump = f"{t['hump_max_mm']:g} mm" if t["hump_max_mm"] is not None else "none"
        front = f"{t['front_mm']} mm" if t["front_mm"] is not None else "no echo"
        return (f"{label} #{t['sequence']} state={t['mission']} motion={t['motion']}(id {t['action_id']}) "
                f"line={t['line']} mapped={'yes' if t['mapped'] else 'NO'} | counts L={t['left']} R={t['right']} "
                f"{motion.update(t['left'], t['right'])} | humps={t['hump_count']} max={hump} | front={front} | "
                f"barcode={t['barcode']}->{t['barcode_action']} | obstacle width={t['obstacle_width_mm']} mm "
                f"clearance={t['obstacle_clearance_mm']} mm | faults: {fault_text(t['faults'])}")
    if kind == EVENT:
        return decode_event(frame)
    if kind == ACK:
        return f"REQUEST ACK seq {frame[1]}: {name(ACK_RESULT, frame[2])}"
    return f"frame type {kind}"


def dashboard_message(frame, motion):
    """One decoded frame as a JSON-ready dict for the web dashboard."""
    if frame_type(frame) == TERRAIN_TRACE:
        return decode_terrain_trace(frame)
    kind = frame_type(frame)
    if kind == WHEEL_TRACE:
        return decode_wheel_trace(frame)
    if kind in (TELEMETRY, SNAPSHOT_REPLY):
        t = decode_telemetry(frame)
        measured = motion.measure(t["left"], t["right"])
        t.update(type="telemetry", faults=active_faults(t["faults"]),
                 speed_left=measured and round(measured[0]), speed_right=measured and round(measured[1]),
                 direction=measured[2] if measured else None,
                 turn_rate_dps=measured and round(turn_rate_dps(measured[0], measured[1])))
        return t
    message = {"type": "event" if kind == EVENT else "ack" if kind == ACK else "other", "text": describe(frame, motion)}
    if kind == EVENT and frame[1] == 2:
        message.update(hump_height_mm=frame[2] * HUMP_MM_PER_CODE, event_id=frame.hex())
    if kind == EVENT and frame[1] == 3:
        message.update(obstacle_clearance_mm=struct.unpack_from("<H", frame, 9)[0],
                       obstacle_side="left" if frame[2] else "right", event_id=frame.hex())
    if kind == ACK:
        message.update(sequence=frame[1], result=name(ACK_RESULT, frame[2]), ok=frame[2] == 0)
    return message


class Dashboard:
    """Serves dashboard.html and pushes decoded frames to browsers as Server-Sent Events."""

    def __init__(self, port, host="127.0.0.1"):
        self.clients, self.lock = [], threading.Lock()
        self.status = None
        page = (Path(__file__).resolve().parent / "dashboard.html").read_bytes()
        self.send_direction = None
        self.send_mapping = None	# set by main once the MQTT client exists; returns the sequence sent
        dashboard = self

        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *args):
                pass

            def do_GET(self):
                if self.path == "/":
                    self.send_response(200)
                    self.send_header("Content-Type", "text/html; charset=utf-8")
                    self.end_headers()
                    self.wfile.write(page)
                elif self.path == "/events":
                    self.send_response(200)
                    self.send_header("Content-Type", "text/event-stream")
                    self.send_header("Cache-Control", "no-cache")
                    self.end_headers()
                    inbox = queue.Queue(maxsize=100)
                    if dashboard.status:
                        inbox.put(dashboard.status)	# the retained online/offline arrives once, often before the page opens
                    with dashboard.lock:
                        dashboard.clients.append(inbox)
                    try:
                        while True:
                            try:
                                data = inbox.get(timeout=15)
                            except queue.Empty:
                                data = None
                            self.wfile.write(b": ping\n\n" if data is None else f"data: {data}\n\n".encode())
                            self.wfile.flush()
                    except OSError:
                        pass  # browser closed the page
                    finally:
                        with dashboard.lock:
                            dashboard.clients.remove(inbox)
                else:
                    self.send_error(404)

            def do_POST(self):
                # Another site open in the same browser could post here; only this page may submit requests.
                if self.path not in ("/map", "/direction") or self.headers.get("Origin") not in origins \
                        or self.headers.get("Content-Type") != "application/json":
                    self.send_error(403)
                    return
                try:
                    body = json.loads(self.rfile.read(min(int(self.headers.get("Content-Length", 0)), 256)))
                    if self.path == "/map":
                        value = mapping_letters(body.get("letters", ""))
                    else:
                        value = body.get("action")
                        if type(value) is not int or not 1 <= value <= 4:
                            value = None
                except (ValueError, AttributeError, TypeError):
                    value = None
                if not value:
                    self.send_error(400, "needs four distinct letters A-Z or a direction action 1..4")
                    return
                sender = dashboard.send_mapping if self.path == "/map" else dashboard.send_direction
                if not sender:
                    self.send_error(503, "not connected to the broker")
                    return
                try:
                    reply = json.dumps({"sequence": sender(value)}).encode()
                except RuntimeError:
                    self.send_error(503, "broker disconnected")
                    return
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(reply)

        # Unauthenticated: keep it on host loopback (the --http-host default).
        self.server = ThreadingHTTPServer((host, port), Handler)
        origins = tuple(f"http://{name}:{self.server.server_port}" for name in ("127.0.0.1", "localhost"))
        self.server.daemon_threads = True
        threading.Thread(target=self.server.serve_forever, daemon=True).start()

    def push(self, message):
        data = json.dumps(message)
        if message.get("type") == "status":
            self.status = data
        with self.lock:
            for inbox in self.clients:
                try:
                    inbox.put_nowait(data)
                except queue.Full:
                    pass	# a stalled browser misses frames; the next telemetry replaces them


def mapping_letters(letters):
    """Four distinct letters A-Z (left, right, straight, U-turn), uppercased; None when invalid."""
    letters = "".join(letters).upper()
    return letters if len(letters) == 4 and len(set(letters)) == 4 and letters.isalpha() and letters.isascii() else None


def mapping_frame(sequence, letters):
    codes = [ord(c) - ord("A") for c in letters]
    packed = sum(code << (5 * i) for i, code in enumerate(codes))
    return bytes([VERSION << 4 | MAPPING, sequence]) + packed.to_bytes(3, "little")


def direction_frame(sequence, action):
    if type(action) is not int or not 1 <= action <= 4:
        raise ValueError("junction direction must be 1..4")
    return bytes([VERSION << 4 | JUNCTION_TEST, sequence, action])


def self_test():
    # 2026-10-31 measured rolling distance must agree with the firmware scale.
    assert abs((5833 + 5816) * 0.5 / COUNTS_PER_MM - 500.0) < 0.01
    # FR-20: right wheel faster by pi/2 * track per second is a 90 deg/s left turn.
    assert abs(turn_rate_dps(0, 3.14159265 / 2 * TRACK_WIDTH_MM) - 90) < 0.01
    assert turn_rate_dps(-50, 50) == -turn_rate_dps(50, -50) > 0 and turn_rate_dps(80, 80) == 0
    golden_trace = bytes.fromhex("38 78 56 34 12 e8 03 0c be e3 03 1d fc 50 b0")
    trace = decode_wheel_trace(golden_trace)
    assert trace["sampled_ms"] == 0x12345678 and trace["output_ok"]
    assert trace["target_left"] == round(1000 / COUNTS_PER_MM, 2)
    assert trace["target_right"] == -trace["target_left"]
    assert trace["measured_right"] == round(-995 / COUNTS_PER_MM, 2)
    assert (trace["duty_left"], trace["duty_right"]) == (80, -80)
    assert dashboard_message(golden_trace, Motion()) == trace
    assert describe(golden_trace, Motion()).startswith("WHEEL ")
    invalid_trace = bytes.fromhex("38 00 00 00 00 00 40 00 60 00 80 00 80 80 80")
    trace = decode_wheel_trace(invalid_trace)
    assert trace["inhibited"] and not trace["output_ok"]
    assert all(trace[k] is None for k in ("target_left", "target_right", "measured_left", "measured_right", "duty_left", "duty_right"))
    for frame in (golden_trace[:-1], golden_trace[:13] + bytes([101, 0]),
                  invalid_trace[:13] + bytes([0, 0])):
        try:
            decode_wheel_trace(frame)
        except ValueError:
            pass
        else:
            raise AssertionError("accepted malformed wheel trace")
    golden_line = bytes.fromhex("39 78 56 34 12 13 22 34 12 78 56 e0 fe e0 64 02 0c")
    line = decode_line_trace(golden_line)
    assert line["sampled_ms"] == 0x12345678 and line["mission"] == "NAVIGATE" and line["phase"] == 2
    assert line["left_black"] and line["right_black"] and line["junction_qualified"] and not line["in_broad"]
    assert (line["left"], line["right"], line["steer_q"], line["law_q"]) == (0x1234, 0x5678, -32, -2)
    assert line["onset_skew_mm"] == 12
    assert (line["command"], line["command_steer_q"], line["command_speed"]) == ("FOLLOW", -32, 100)
    assert describe(golden_line, Motion()).startswith("LINE ")
    assert describe(bytes([0x3B, 5, 0, 14, 0xF9, 3, 100, 0xFA, 1, 0xFF, 62, 9, 0x34, 0x12]), Motion()).startswith("HUMP t=5 pitch=3.5 roll=-1.5 state=1 rise=3mm accel=1000mg x=16 y=-16 z=992 height=9mm travel=4660mm encoder_ok=1 calibrated=1")
    assert describe(bytes([0x3A, 7, 0x22, 0x80, 0x0C, 0]) + bytes(12), Motion()) == "BARS #7 H34 L12"
    assert decode_line_trace(golden_line[:5] + bytes([0x80]) + golden_line[6:])["barcode_level"]
    assert decode_line_trace(golden_line[:15] + bytes([0x0A]) + golden_line[16:])["barcode_edges"] == 1
    for frame in (golden_line[:-1], golden_line[:6] + bytes([0x07]) + golden_line[7:]):
        try:
            decode_line_trace(frame)
        except ValueError:
            pass
        else:
            raise AssertionError("accepted malformed line trace")
    t = decode_telemetry(GOLDEN_TELEMETRY)
    assert frame_type(GOLDEN_TELEMETRY) == TELEMETRY and t["sequence"] == 0x2A and t["mission"] == "NAVIGATE"
    assert t["motion"] == "FAILED" and t["line"] == "VISIBLE" and t["action_id"] == 5 and t["mapped"]
    assert t["faults"] == [1, 2, 4, 1, 1, 3] and (t["left"], t["right"]) == (0x1234, 0x5678)
    assert t["hump_count"] == 3 and t["front_mm"] == 0x10D0 and t["barcode"] == "B"
    assert t["barcode_action"] == "STRAIGHT" and (t["obstacle_width_mm"], t["obstacle_clearance_mm"]) == (420, 170)
    assert "BARCODE: letter Z -> RIGHT" in decode_event(GOLDEN_EVENT)
    assert mapping_frame(7, "ABCD") == GOLDEN_MAPPING
    assert direction_frame(7, 4) == bytes([0x37, 7, 4])
    assert mapping_letters("abcd") == "ABCD" and mapping_letters(["W", "F", "a", "Z"]) == "WFAZ"
    assert not any(mapping_letters(bad) for bad in ("ABCA", "ABC", "ABCDE", "AB1D", "ABÇD", ""))
    obstacle = bytes([0x32, 3, 0]) + struct.pack("<HHHH", 10, 20, 30, 40)
    event = dashboard_message(obstacle, Motion())
    assert event["type"] == "event" and event["obstacle_clearance_mm"] == 40
    assert event["event_id"] == obstacle.hex() and "OBSTACLE" in event["text"]
    assert "obstacle_clearance_mm" not in dashboard_message(GOLDEN_EVENT, Motion())
    trace = bytes([0x3B, 5, 0, 14, 0xF9, 3, 100, 0xFA, 1, 0xFF, 62, 9, 0x34, 0x12])
    terrain = dashboard_message(trace, Motion())
    assert terrain["type"] == "terrain_trace" and terrain["pitch_deg"] == 3.5 and terrain["roll_deg"] == -1.5
    assert (terrain["x_mg"], terrain["y_mg"], terrain["z_mg"]) == (16, -16, 992)
    assert (terrain["height_mm"], terrain["rise_mm"], terrain["travel_mm"]) == (9, 3, 0x1234)
    hump = bytes([GOLDEN_EVENT[0], 2, 0]) + GOLDEN_EVENT[3:]
    assert dashboard_message(hump, Motion())["hump_height_mm"] == 0
    ack = dashboard_message(bytes([0x35, 0x07, 0x01]), Motion())
    assert ack["type"] == "ack" and ack["sequence"] == 7 and not ack["ok"] and ack["result"].startswith("BUSY")
    assert [len(bits) for _, bits in FAULTS] == header_fault_widths(), "FAULTS disagrees with robot_data.h"
    position = 0
    for owner, (_, bits) in enumerate(FAULTS):
        for bit in range(len(bits)):
            expected = [0] * len(FAULTS)
            expected[owner] = 1 << bit
            assert unpack_faults(1 << position) == expected
            position += 1
    assert position == 12 and unpack_faults(0xFFF) == [3, 7, 7, 1, 1, 3]
    bad_telemetry = []
    for index, value in ((3, GOLDEN_TELEMETRY[3] | 0x80), (5, GOLDEN_TELEMETRY[5] | 0x10),
                         (13, GOLDEN_TELEMETRY[13] | 0xC0),
                         (14, 26), (14, 5 << 5), (11, 0xFF)):
        frame = bytearray(GOLDEN_TELEMETRY)
        frame[index] = value
        bad_telemetry.append(frame)
    frame = bytearray(GOLDEN_TELEMETRY)
    frame[12:14] = (0x3FFF).to_bytes(2, "little")
    bad_telemetry.append(frame)
    bad_events = [bytes([0x32, kind]) + GOLDEN_EVENT[2:] for kind in (0, 5, 0x11)]
    for frame in bad_telemetry + bad_events:
        history = Motion()
        try:
            dashboard_message(bytes(frame), history)
        except ValueError:
            pass
        else:
            raise AssertionError(f"accepted malformed frame {frame.hex()}")
        assert history.last is None
    # Valid zero is a measurement; the invalid sentinel without validity is absent.
    frame = bytearray(GOLDEN_TELEMETRY)
    frame[12:14] = (0x2000).to_bytes(2, "little")
    assert decode_telemetry(frame)["front_mm"] == 0
    frame[12:14] = (0x1FFF).to_bytes(2, "little")
    assert decode_telemetry(frame)["front_mm"] is None
    assert decode_telemetry(GOLDEN_TELEMETRY)["imu_calibrated"] is None
    for kind in (TELEMETRY, SNAPSHOT_REPLY):
        for status, expected in ((0, None), (1, False), (2, True)):
            frame = bytearray(GOLDEN_TELEMETRY)
            frame[0] = VERSION << 4 | kind
            frame[10:12] = bytes([0, 0x7F])  # No completed hump.
            frame[12:14] = (status << 14 | 0x2000).to_bytes(2, "little")
            decoded = decode_telemetry(frame)
            assert decoded["imu_calibrated"] is expected
            assert decoded["hump_max_mm"] is None and decoded["front_mm"] == 0
            frame[12:14] = (status << 14 | 0x3FFF).to_bytes(2, "little")
            try:
                decode_telemetry(frame)
            except ValueError:
                pass
            else:
                raise AssertionError("calibration hid invalid front range")
    print("mqtt_client self-test: PASS")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("action", nargs="?", choices=("monitor", "map", "snapshot", "serve"))
    parser.add_argument("letters", nargs="*", help="map: four distinct letters for left, right, straight, U-turn")
    parser.add_argument("--broker", help="broker address")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--root", default="robot/s3", help="topic root (firmware: <root>/telemetry, /request, /status)")
    parser.add_argument("--http-port", type=int, default=8080, help="serve: dashboard port")
    parser.add_argument("--http-host", default="127.0.0.1", help="serve: bind address")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--raw", action="store_true",
                        help="monitor: print every frame as JSON, without folding repeats")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if not args.action or not args.broker:
        parser.error("an action and --broker are required")
    letters = mapping_letters(args.letters)
    if args.action == "map" and not letters:
        parser.error("map needs four distinct letters A-Z, e.g. map A B C D")
    try:
        import paho.mqtt.client as mqtt
    except ImportError:
        parser.exit(1, "Install: python -m pip install -r scripts/requirements-mqtt.txt\n")

    ready, done = threading.Event(), threading.Event()
    exit_code = 1
    sequence = int(time.time()) & 0xFF
    motion = Motion()
    dashboard = Dashboard(args.http_port, args.http_host) if args.action == "serve" else None
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

    folder = None
    if args.action == "monitor" and not args.raw:
        from compact_log import Folder  # same folder as this script
        folder = Folder()

    def on_message(client, userdata, message):
        nonlocal exit_code
        if dashboard and message.topic.endswith("/status"):
            dashboard.push({"type": "status", "online": message.payload == b"1"})
            return
        if dashboard:
            try:
                frame = bytes(message.payload)
                if frame_type(frame) not in (LINE_TRACE, BARCODE_TRACE):	# monitor captures only; no dashboard view
                    dashboard.push(dashboard_message(frame, motion))
            except ValueError as error:
                dashboard.push({"type": "other", "text": f"undecodable frame ({error})"})
            return
        if message.topic.endswith("/status"):
            print(f"STATUS {message.payload.decode('utf-8', 'replace')}", flush=True)
            return
        try:
            line = describe(bytes(message.payload), motion)
        except ValueError as error:
            print(f"undecodable frame ({error}): {message.payload.hex()}", flush=True)
            return
        if args.action == "monitor":
            for text in ([line] if folder is None else folder.feed(line)):
                print(text, flush=True)
        elif args.action == "map" and frame_type(message.payload) == ACK and message.payload[1] == sequence:
            print(line, flush=True)
            exit_code = 0 if message.payload[2] == 0 else 1
            done.set()
        elif args.action == "snapshot" and line.startswith("SNAPSHOT"):
            print(line, flush=True)
            exit_code = 0
            done.set()

    client.on_connect = on_connect
    client.on_message = on_message
    try:
        client.connect(args.broker, args.port, keepalive=15)
        client.loop_start()
        if not ready.wait(8):
            raise RuntimeError("timed out connecting to the broker")
        if args.action in ("monitor", "serve"):
            if dashboard:
                send_lock = threading.Lock()
                def send_request(value, encode):
                    nonlocal sequence
                    with send_lock:
                        if not client.is_connected():
                            raise RuntimeError("broker disconnected")
                        sequence = (sequence + 1) & 0xFF
                        client.publish(args.root + "/request", encode(sequence, value), qos=1)
                        return sequence
                dashboard.send_mapping = lambda letters: send_request(letters, mapping_frame)
                dashboard.send_direction = lambda action: send_request(action, direction_frame)
                print(f"Dashboard: http://127.0.0.1:{args.http_port}/ ; Ctrl+C to exit.", flush=True)
            else:
                print("Monitoring; Ctrl+C to exit.", flush=True)
            done.wait()
            return 0
        payload = mapping_frame(sequence, letters) if args.action == "map" else bytes([VERSION << 4 | SNAPSHOT_REQUEST, sequence])
        client.publish(args.root + "/request", payload, qos=1).wait_for_publish(timeout=5)
        if not done.wait(8):
            raise RuntimeError("no reply from the robot (is it online and stopped?)")
        return exit_code
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

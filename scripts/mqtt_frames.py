"""Binary MQTT frames, schema version 1 (docs/MQTT_PROTOCOL.md). Pure Python, no I/O."""

VERSION = 1
TELEMETRY, EVENT, SNAPSHOT_REPLY, MAPPING, ACK, SNAPSHOT_REQUEST = 1, 2, 3, 4, 5, 6
LENGTHS = {TELEMETRY: 18, EVENT: 11, SNAPSHOT_REPLY: 18, MAPPING: 5, ACK: 3, SNAPSHOT_REQUEST: 2}
MOTION_STATUS = ("IDLE", "RUNNING", "DONE", "FAILED")
ACTIONS = ("UNMAPPED", "LEFT", "RIGHT", "STRAIGHT", "U_TURN")
MISSION_STATES = ("CALIBRATE", "READY", "FOLLOW", "JUNCTION_TURN", "BYPASS", "COMPLETE", "STOP")
FAULT_OWNERS = ("motion", "line", "terrain", "obstacle", "network")
EVENT_KINDS = {1: "barcode", 2: "hump", 3: "obstacle", 4: "turn"}
ACK_RESULTS = ("installed", "busy", "invalid letters", "malformed")


class FrameError(ValueError):
    """The frame is not valid schema-1 data; the message says why."""


def _header(frame, expect=None):
    if not frame:
        raise FrameError("empty frame")
    if frame[0] >> 4 != VERSION:
        raise FrameError(f"unsupported version {frame[0] >> 4}")
    kind = frame[0] & 0xF
    if kind not in LENGTHS:
        raise FrameError(f"unknown type {kind}")
    if len(frame) != LENGTHS[kind]:
        raise FrameError(f"type {kind} needs {LENGTHS[kind]} bytes, got {len(frame)}")
    if expect is not None and kind not in expect:
        raise FrameError(f"unexpected type {kind}")
    return kind


def _le16(frame, at):
    return frame[at] | (frame[at + 1] << 8)


def decode_telemetry(frame):
    kind = _header(frame, (TELEMETRY, SNAPSHOT_REPLY))
    if frame[2] & 0xC0 or frame[3] & 0xC0 or frame[16] & 0xC0:
        raise FrameError("reserved bit set")
    letter = frame[17] & 0x1F
    if letter > 25 and letter != 31:
        raise FrameError(f"barcode letter {letter} out of range")
    front = _le16(frame, 15)
    return {
        "type": "snapshot" if kind == SNAPSHOT_REPLY else "telemetry",
        "sequence": frame[1],
        "mission_state": frame[2] & 0xF,
        "motion_status": MOTION_STATUS[(frame[2] >> 4) & 3],
        "motion_action": frame[3] & 0x3F,
        "faults": dict(zip(FAULT_OWNERS, frame[4:9])),
        "left_count": _le16(frame, 9),
        "right_count": _le16(frame, 11),
        "hump_count": frame[13],
        "hump_max_code": frame[14] & 0x7F,
        "hump_valid": bool(frame[14] & 0x80),
        "front_mm": front & 0x1FFF,
        "front_valid": bool(front & 0x2000),
        "barcode_letter": None if letter == 31 else chr(ord("A") + letter),
        "barcode_action": ACTIONS[min(frame[17] >> 5, len(ACTIONS) - 1)] if (frame[17] >> 5) < len(ACTIONS) else f"action{frame[17] >> 5}",
    }


def decode_event(frame):
    _header(frame, (EVENT,))
    if frame[1] & 0xF0:
        raise FrameError("reserved bit set")
    kind = frame[1] & 0xF
    if kind not in EVENT_KINDS:
        raise FrameError(f"event kind {kind} out of range")
    return {"type": "event", "kind": EVENT_KINDS[kind], "arg": frame[2], "time_ms": _le16(frame, 3),
            "left_count": _le16(frame, 5), "right_count": _le16(frame, 7), "arg2": _le16(frame, 9)}


def decode_ack(frame):
    _header(frame, (ACK,))
    if frame[2] > 3:
        raise FrameError(f"ack result {frame[2]} out of range")
    return {"type": "ack", "sequence": frame[1], "result": ACK_RESULTS[frame[2]]}


def decode(frame):
    """Decode any robot-to-PC frame into a readable dict."""
    kind = _header(frame)
    if kind in (TELEMETRY, SNAPSHOT_REPLY):
        return decode_telemetry(frame)
    if kind == EVENT:
        return decode_event(frame)
    if kind == ACK:
        return decode_ack(frame)
    raise FrameError(f"type {kind} is a PC-to-robot command")


def encode_mapping(sequence, letters):
    """letters: four distinct characters A-Z for left, right, straight, U-turn."""
    if len(letters) != 4 or any(not ("A" <= c <= "Z") for c in letters) or len(set(letters)) != 4:
        raise FrameError("mapping needs four distinct letters A-Z")
    if not 0 <= sequence <= 255:
        raise FrameError("sequence must be 0..255")
    packed = sum((ord(c) - 65) << (5 * i) for i, c in enumerate(letters))
    return bytes([(VERSION << 4) | MAPPING, sequence, packed & 0xFF, (packed >> 8) & 0xFF, (packed >> 16) & 0xFF])


def encode_snapshot_request(sequence):
    if not 0 <= sequence <= 255:
        raise FrameError("sequence must be 0..255")
    return bytes([(VERSION << 4) | SNAPSHOT_REQUEST, sequence])

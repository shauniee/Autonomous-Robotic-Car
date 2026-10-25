"""Monitor binary S1 telemetry, request a snapshot or send the barcode mapping. Python 3.10+.

  python scripts/mqtt_client.py selftest
  python scripts/mqtt_client.py monitor  --broker 192.168.1.10
  python scripts/mqtt_client.py snapshot --broker 192.168.1.10
  python scripts/mqtt_client.py mapping  --broker 192.168.1.10 --letters AZWF   # left right straight u-turn
"""
import argparse
import json
import os
import sys
import threading
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mqtt_frames as frames  # noqa: E402

GOLDEN = {
    "telemetry": "112A320501020408033412785603F8D03061",
    "event": "120119E803100011000200",
    "ack": "150700",
}


def selftest():
    """Decode the golden vectors of docs/MQTT_PROTOCOL.md and reject malformed frames."""
    tel = frames.decode(bytes.fromhex(GOLDEN["telemetry"]))
    assert tel["sequence"] == 42 and tel["motion_status"] == "FAILED" and tel["left_count"] == 0x1234
    assert tel["front_mm"] == 0x10D0 and tel["front_valid"] and tel["barcode_letter"] == "B"
    assert frames.decode(bytes.fromhex(GOLDEN["event"]))["kind"] == "barcode"
    assert frames.decode(bytes.fromhex(GOLDEN["ack"]))["result"] == "installed"
    assert frames.encode_mapping(7, "ABCD").hex() == "1407208801"
    assert frames.encode_snapshot_request(9).hex() == "1609"
    bad = [b"", bytes.fromhex(GOLDEN["telemetry"])[:-1], b"\x21" + bytes(17), b"\x1f" + bytes(17),
           bytes.fromhex(GOLDEN["telemetry"][:4] + "72" + GOLDEN["telemetry"][6:]), bytes.fromhex("150709")]
    for frame in bad:
        try:
            frames.decode(frame)
        except frames.FrameError:
            continue
        raise AssertionError(f"accepted malformed frame {frame.hex()}")
    for letters in ("ABC", "AABC", "ABC1", "abcd"):
        try:
            frames.encode_mapping(1, letters)
        except frames.FrameError:
            continue
        raise AssertionError(f"accepted letters {letters}")
    print("selftest: PASS")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("action", choices=("monitor", "snapshot", "mapping", "selftest"))
    parser.add_argument("--broker", help="Broker LAN IPv4 address")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--root", default="robot/s3")
    parser.add_argument("--letters", help="Four distinct letters A-Z: left right straight u-turn")
    parser.add_argument("--timeout", type=float, default=10.0, help="Seconds to wait for a reply")
    args = parser.parse_args()
    if args.action == "selftest":
        selftest()
        return
    if not args.broker:
        parser.error("--broker is required")
    if not 1 <= args.port <= 65535:
        parser.error("Port must be 1..65535")
    if not args.root or any(c in args.root for c in "+#\0") or len(args.root.encode()) > 80:
        parser.error("Root must be 1..80 bytes without MQTT wildcards or NUL")
    request = None
    if args.action == "snapshot":
        request = frames.encode_snapshot_request(1)
    elif args.action == "mapping":
        if not args.letters:
            parser.error("--letters is required for mapping")
        try:
            request = frames.encode_mapping(1, args.letters.upper())
        except frames.FrameError as error:
            parser.error(str(error))
    try:
        import paho.mqtt.client as mqtt
    except ImportError:
        parser.exit(1, "Install: python -m pip install -r scripts/requirements-mqtt.txt\n")

    done = threading.Event()
    state = {"error": None}
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.connect_timeout = 5.0
    username = os.getenv("ROBOT_MQTT_USERNAME")
    if username:
        client.username_pw_set(username, os.getenv("ROBOT_MQTT_PASSWORD", ""))

    def on_connect(client, userdata, flags, reason_code, properties):
        if reason_code.is_failure:
            state["error"] = f"Broker refused the connection: {reason_code}"
            done.set()
            return
        client.subscribe([(f"{args.root}/telemetry", 0), (f"{args.root}/status", 1)])
        if request is not None:
            client.publish(f"{args.root}/request", request, qos=0)

    def on_message(client, userdata, message):
        if message.topic.endswith("/status"):
            print("robot", "online" if message.payload == b"1" else "offline")
            return
        try:
            decoded = frames.decode(bytes(message.payload))
        except frames.FrameError as error:
            print(f"rejected frame ({error}): {bytes(message.payload).hex()}", file=sys.stderr)
            return
        print(json.dumps(decoded, sort_keys=True))
        wanted = {"snapshot": "snapshot", "mapping": "ack"}.get(args.action)
        if wanted and decoded["type"] == wanted:
            done.set()

    client.on_connect = on_connect
    client.on_message = on_message
    try:
        client.connect(args.broker, args.port)
    except OSError as error:
        parser.exit(1, f"Cannot reach the broker: {error}\n")
    client.loop_start()
    try:
        if args.action == "monitor":
            while not state["error"]:
                time.sleep(0.5)
        elif not done.wait(args.timeout):
            state["error"] = "No reply from the robot before the timeout"
    except KeyboardInterrupt:
        pass
    finally:
        client.loop_stop()
        client.disconnect()
    if state["error"]:
        parser.exit(1, state["error"] + "\n")


if __name__ == "__main__":
    main()

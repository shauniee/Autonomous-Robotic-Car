"""Monitor telemetry, request status or load four barcode assignments. Python 3.10+."""
import argparse
import json
import os
import threading
import time


def read_mapping(path):
    def unique_keys(pairs):
        values = {}
        for key, value in pairs:
            if key in values:
                raise ValueError(f"Duplicate letter: {key}")
            values[key] = value
        return values

    with open(path, encoding="utf-8") as source:
        values = json.load(source, object_pairs_hook=unique_keys)
    alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    motions = {"left", "right", "straight", "u_turn"}
    if not isinstance(values, dict) or len(values) not in (4, 26) or any(
        key not in alphabet or len(key) != 1 for key in values
    ):
        raise ValueError("Use four distinct A-Z letters, or a complete A-Z table")
    if any(not isinstance(value, str) or value not in motions | {"unmapped"}
           for value in values.values()):
        raise ValueError("Actions must be left, right, straight, u_turn or unmapped")
    assigned = [value for value in values.values() if value != "unmapped"]
    if len(assigned) != 4 or set(assigned) != motions:
        raise ValueError("Assign exactly one distinct letter to each of the four motions")
    return [values.get(letter, "unmapped") for letter in alphabet]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("monitor", "status", "mapping", "events"))
    parser.add_argument("--broker", help="Broker LAN IPv4 address; required for network actions")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--root", default="robot/s3")
    parser.add_argument("--file", help="Four letter/action assignments, or a full A-Z JSON table")
    parser.add_argument("--validate-only", action="store_true",
                        help="Validate a mapping and print its 26-entry payload locally")
    parser.add_argument("--kind", choices=("hump", "barcode"), default="hump")
    parser.add_argument("--after-id", type=int, default=0)
    args = parser.parse_args()
    if not 0 <= args.after_id <= 4294967295:
        parser.error("after-id must be 0..4294967295")
    request_id = (time.time_ns() // 1000000) % 4294967295 or 1
    mapping = None
    if args.action == "mapping":
        if not args.file:
            parser.error("mapping requires --file")
        try:
            mapping = read_mapping(args.file)
        except (OSError, ValueError) as error:
            parser.error(str(error))
    if args.validate_only and args.action != "mapping":
        parser.error("--validate-only requires the mapping action")
    if not args.validate_only and not args.broker:
        parser.error("Network actions require --broker")
    if not 1 <= args.port <= 65535:
        parser.error("Port must be 1..65535")
    if not args.root or any(c in args.root for c in "+#\0") or len(args.root.encode()) > 80:
        parser.error("Root must be 1..80 bytes without MQTT wildcards or NUL")
    mapping_payload = json.dumps({"request": "barcode_mapping", "request_id": request_id,
                                  "mapping": mapping}, separators=(",", ":")) if mapping is not None else None
    if args.validate_only:
        print(mapping_payload)
        print("Mapping validated locally: four assigned letters, 22 unmapped.")
        return 0
    try:
        import paho.mqtt.client as mqtt
    except ImportError:
        parser.exit(1, "Install: python -m pip install -r scripts/requirements-mqtt.txt\n")

    subscribed = threading.Event()
    received = threading.Event()
    state = {"error": None, "heartbeat_at": time.monotonic(), "status": None}
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.connect_timeout = 5.0
    username = os.getenv("ROBOT_MQTT_USERNAME")
    if username:
        client.username_pw_set(username, os.getenv("ROBOT_MQTT_PASSWORD", ""))

    def fail(message):
        state["error"] = message
        subscribed.set()
        received.set()

    def on_connect(client, userdata, flags, reason_code, properties):
        if reason_code.is_failure:
            fail(f"Broker refused connection: {reason_code}")
            return
        topic = args.root + ("/#" if args.action == "monitor" else
            "/mapping_status" if args.action == "mapping" else
            "/events" if args.action == "events" else "/status")
        result, _ = client.subscribe(topic, qos=1)
        if result != mqtt.MQTT_ERR_SUCCESS:
            fail("Could not subscribe")

    def on_subscribe(client, userdata, mid, reason_codes, properties):
        if any(code.is_failure for code in reason_codes):
            fail("Broker refused subscription")
        else:
            subscribed.set()

    def on_disconnect(client, userdata, flags, reason_code, properties):
        if reason_code.is_failure:
            fail(f"Broker connection lost: {reason_code}")

    def on_message(client, userdata, message):
        text = message.payload.decode("utf-8", errors="replace")
        if args.action == "monitor":
            retained = " [retained]" if message.retain else ""
            print(f"{message.topic}{retained} {text}", flush=True)
        try:
            payload = json.loads(message.payload)
        except (ValueError, UnicodeDecodeError):
            return
        if args.action == "mapping" and isinstance(payload, dict) and not message.retain and \
            payload.get("request_id") == request_id:
            state["status"] = text
            if payload.get("result") != 2:
                fail("Pico rejected mapping: " + text)
            else:
                received.set()
            return
        if args.action == "events" and isinstance(payload, dict) and not message.retain and \
            payload.get("schema") == 3 and payload.get("kind") == args.kind and \
            payload.get("after_id") == args.after_id:
            state["status"] = text
            received.set()
            return
        if not isinstance(payload, dict) or payload.get("online") is not True:
            return
        if not message.retain and message.topic == args.root + "/heartbeat":
            state["heartbeat_at"] = time.monotonic()
        if not message.retain and message.topic == args.root + "/status" and \
            payload.get("schema") == 3 and isinstance(payload.get("mission"), dict):
            state["status"] = text
            received.set()

    client.on_connect = on_connect
    client.on_subscribe = on_subscribe
    client.on_disconnect = on_disconnect
    client.on_message = on_message
    running = False
    try:
        client.connect(args.broker, args.port, keepalive=15)
        client.loop_start()
        running = True
        if not subscribed.wait(8):
            raise RuntimeError("Timed out connecting/subscribing to broker")
        if state["error"]:
            raise RuntimeError(state["error"])
        if args.action == "monitor":
            print("Monitoring; Ctrl+C to exit. Expect a live heartbeat every second.", flush=True)
            stale_reported = False
            while True:
                if state["error"]:
                    raise RuntimeError(state["error"])
                stale = time.monotonic() - state["heartbeat_at"] > 3
                if stale and not stale_reported:
                    print("Heartbeat unavailable for >3 s; retained status may be stale.", flush=True)
                stale_reported = stale
                time.sleep(0.25)
        payload = mapping_payload if mapping is not None else json.dumps(
            {"request":"events", "kind":args.kind, "after_id":args.after_id},
            separators=(",", ":")) if args.action == "events" else '{"request":"status"}'
        result = client.publish(args.root + ("/mapping" if mapping is not None else "/request"),
            payload, qos=1, retain=False)
        if result.rc != mqtt.MQTT_ERR_SUCCESS:
            raise RuntimeError("Could not publish status request")
        result.wait_for_publish(timeout=5)
        if not result.is_published():
            raise RuntimeError("Broker did not acknowledge request")
        if not received.wait(8):
            raise RuntimeError("No live status observed; check Pico connection/subscription")
        if state["error"]:
            raise RuntimeError(state["error"])
        print(state["status"])
        print("Pico acknowledgement observed; mapping is RAM-only." if mapping is not None else
            "Live event history observed." if args.action == "events" else
            "Live robot status observed; broker acknowledgement alone is not a Pico response.")
    except KeyboardInterrupt:
        return 0
    except (OSError, RuntimeError, ValueError) as error:
        print(f"Error: {error}")
        return 1
    finally:
        client.disconnect()
        if running:
            client.loop_stop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

"""Monitor a car, send a fresh session-bound start, or send stop. Python 3.10+."""
import argparse
import json
import os
import re
import threading
import time


def start_payload(heartbeat):
    """Use a just-received, non-retained heartbeat from a single command operator."""
    if not isinstance(heartbeat, dict) or heartbeat.get("online") is not True:
        raise ValueError("No online heartbeat")
    session = heartbeat.get("session")
    if not isinstance(session, str) or not re.fullmatch(r"[0-9a-f]{16}", session):
        raise ValueError("Invalid session")
    uptime = heartbeat.get("uptime_ms")
    last_id = heartbeat.get("last_command_id")
    if type(uptime) is not int or not 0 <= uptime <= 0xFFFFFFFF:
        raise ValueError("Invalid board uptime")
    if type(last_id) is not int or not 0 <= last_id < 0xFFFFFFFF:
        raise ValueError("Invalid/exhausted command ID; reconnect the car")
    return json.dumps({"session": session, "id": last_id + 1,
                       "issued_ms": uptime, "command": "start"}, separators=(",", ":"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("monitor", "start", "stop"))
    parser.add_argument("--broker", required=True, help="Broker LAN IP")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--root", default="robot/car01")
    parser.add_argument("--username", default=os.getenv("ROBOT_MQTT_USERNAME"))
    args = parser.parse_args()
    try:
        import paho.mqtt.client as mqtt
    except ImportError:
        parser.exit(1, "Install: python -m pip install -r scripts/requirements-mqtt.txt\n")

    connected = threading.Event()
    heartbeat_ready = threading.Event()
    state = {"error": None, "start": None, "received_at": 0.0}
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.connect_timeout = 5.0
    if args.username:
        client.username_pw_set(args.username, os.getenv("ROBOT_MQTT_PASSWORD", ""))

    def on_connect(client, userdata, flags, reason_code, properties):
        if reason_code.is_failure:
            state["error"] = f"Broker refused connection: {reason_code}"
        elif args.action != "stop":
            topic = args.root + ("/#" if args.action == "monitor" else "/heartbeat")
            result, _ = client.subscribe(topic, qos=1)
            if result != mqtt.MQTT_ERR_SUCCESS:
                state["error"] = "Could not subscribe"
        connected.set()

    def on_subscribe(client, userdata, mid, reason_codes, properties):
        if any(code.is_failure for code in reason_codes):
            state["error"] = "Broker refused subscription"
            heartbeat_ready.set()

    def on_message(client, userdata, message):
        if args.action == "monitor":
            print(message.topic, message.payload.decode("utf-8", errors="replace"), flush=True)
        elif not message.retain and message.topic == args.root + "/heartbeat":
            try:
                payload = start_payload(json.loads(message.payload))
            except (ValueError, TypeError, UnicodeDecodeError):
                return
            state["start"] = payload
            state["received_at"] = time.monotonic()
            heartbeat_ready.set()

    client.on_connect = on_connect
    client.on_subscribe = on_subscribe
    client.on_message = on_message
    running = False
    try:
        client.connect(args.broker, args.port, keepalive=15)
        client.loop_start()
        running = True
        if not connected.wait(8):
            raise RuntimeError("Timed out connecting to broker")
        if state["error"]:
            raise RuntimeError(state["error"])
        if args.action == "monitor":
            print("Monitoring; Ctrl+C to exit. Heartbeat should arrive every second.", flush=True)
            while True:
                if state["error"]:
                    raise RuntimeError(state["error"])
                time.sleep(0.25)
        payload = "stop"
        if args.action == "start":
            if not heartbeat_ready.wait(8):
                raise RuntimeError("No fresh heartbeat; check the car's WiFi/MQTT connection")
            if state["error"]:
                raise RuntimeError(state["error"])
            if not state["start"] or time.monotonic() - state["received_at"] > 2:
                raise RuntimeError("Heartbeat became stale; retry start")
            payload = state["start"]
        result = client.publish(args.root + "/command", payload, qos=1, retain=False)
        result.wait_for_publish(timeout=5)
        if not result.is_published():
            raise RuntimeError("Broker did not acknowledge command")
        print("Command delivered to broker. Check car telemetry for the resulting state.")
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

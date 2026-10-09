"""Monitor S1 telemetry/heartbeats or request read-only status. Python 3.10+."""
import argparse
import json
import os
import threading
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("monitor", "status"))
    parser.add_argument("--broker", required=True, help="Broker LAN IPv4 address")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--root", default="robot/s3")
    args = parser.parse_args()
    if not 1 <= args.port <= 65535:
        parser.error("Port must be 1..65535")
    if not args.root or any(c in args.root for c in "+#\0") or len(args.root.encode()) > 80:
        parser.error("Root must be 1..80 bytes without MQTT wildcards or NUL")
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
        topic = args.root + ("/#" if args.action == "monitor" else "/status")
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
        if not isinstance(payload, dict) or payload.get("online") is not True:
            return
        if not message.retain and message.topic == args.root + "/heartbeat":
            state["heartbeat_at"] = time.monotonic()
        if not message.retain and message.topic == args.root + "/status":
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
        result = client.publish(args.root + "/request", '{"request":"status"}',
                                qos=1, retain=False)
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
        print("Live status observed; broker acknowledgement alone is not a Pico response.")
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

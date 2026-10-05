"""Live terminal statistics for sensor_bench_mqtt. No remote movement controls."""
import argparse
import json
import os
import sys
import threading
import time


def statistics(data):
    if data.get("mode") != "sensor_bench":
        return "Different firmware mode: " + json.dumps(data)
    ir = data.get("ir_right", {})
    imu = data.get("imu", {})
    sonar = data.get("ultrasonic", {})
    servo = data.get("servo", {})
    def axes(v):
        return f"{v.get('x')} / {v.get('y')} / {v.get('z')} ({v.get('unit')}, valid={v.get('valid')})"
    return "\n".join([
        f"Uptime: {data.get('uptime_ms', 0) / 1000:.1f} s; drive motors disabled",
        f"Right IR GP26: {ir.get('raw_adc')} / 4095; black fraction={ir.get('black_fraction')}",
        "Acceleration X/Y/Z: " + axes(imu.get("acceleration", {})),
        "Magnetic X/Y/Z: " + axes(imu.get("magnetic_field", {})),
        f"Gravity pitch/roll: {imu.get('pitch_deg')} / {imu.get('roll_deg')} deg",
        f"Ultrasonic latest: {sonar.get('latest', {}).get('distance_m')} m; forward={sonar.get('forward', {}).get('distance_m')} m",
        f"Servo: {servo.get('commanded_angle_deg')} deg commanded; enabled={servo.get('enabled')}; scan={servo.get('scan_enabled')}",
    ])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--broker", required=True, help="Broker LAN IPv4 address")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--root", default="robot/car01")
    parser.add_argument("--json", action="store_true", help="Print every raw message")
    parser.add_argument("--username", default=os.getenv("ROBOT_MQTT_USERNAME"))
    args = parser.parse_args()
    try:
        import paho.mqtt.client as mqtt
    except ImportError:
        parser.exit(1, "Install dependencies: python -m pip install -r scripts/requirements-mqtt.txt\n")
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.connect_timeout = 5.0
    if args.username:
        client.username_pw_set(args.username, os.getenv("ROBOT_MQTT_PASSWORD", ""))
    lock = threading.Lock()
    state = {"broker": "connecting", "online": None, "telemetry": None, "last": 0, "error": None}
    def on_connect(client, userdata, flags, reason, properties):
        with lock:
            state["broker"] = "refused" if reason.is_failure else "connected"
            if reason.is_failure:
                state["error"] = str(reason)
        if not reason.is_failure:
            client.subscribe(args.root + "/#", qos=1)
    def on_disconnect(client, userdata, flags, reason, properties):
        with lock:
            state["broker"] = "disconnected; retrying"
    def on_subscribe(client, userdata, mid, reasons, properties):
        if any(reason.is_failure for reason in reasons):
            with lock:
                state["error"] = "Broker refused subscription"
    def on_message(client, userdata, msg):
        if args.json:
            print(msg.topic, msg.payload.decode("utf-8", errors="replace"), flush=True)
        try:
            data = json.loads(msg.payload)
            if not isinstance(data, dict):
                return
        except (ValueError, UnicodeDecodeError):
            return
        with lock:
            if msg.topic.endswith("/status") or msg.topic.endswith("/heartbeat"):
                state["online"] = data.get("online")
            if msg.topic == args.root + "/telemetry":
                state["telemetry"] = data
                state["last"] = time.monotonic()
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_subscribe = on_subscribe
    client.on_message = on_message
    try:
        client.connect(args.broker, args.port, keepalive=15)
        client.loop_start()
        while True:
            with lock:
                snapshot = dict(state)
            if snapshot["error"]:
                raise RuntimeError(snapshot["error"])
            if not args.json:
                age = time.monotonic() - snapshot["last"] if snapshot["last"] else None
                fresh = age is not None and age < 3 and snapshot["online"] is not False
                output = f"Broker: {snapshot['broker']}  Robot data: {'LIVE' if fresh else 'STALE / WAITING'}\n"
                if snapshot["telemetry"]:
                    output += f"Last telemetry received {age:.1f} s ago\n" + statistics(snapshot["telemetry"])
                else:
                    output += "Waiting for robot telemetry; check WiFi, broker IP and flashed firmware."
                if sys.stdout.isatty():
                    print("\033[2J\033[H" + output, end="\n", flush=True)
                else:
                    print(output, flush=True)
            time.sleep(1)
    except KeyboardInterrupt:
        return 0
    except (OSError, RuntimeError, ValueError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1
    finally:
        client.disconnect()
        client.loop_stop()


if __name__ == "__main__":
    raise SystemExit(main())

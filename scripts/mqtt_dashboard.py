"""Show robot MQTT readings at http://127.0.0.1:8080. Python 3.9+."""
import argparse
import errno
import json
import math
import os
from pathlib import Path
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlsplit


ASSETS = Path(__file__).resolve().parent / "dashboard"
TOPICS = ("telemetry", "heartbeat", "status", "mapping_status", "events")
MAX_PAYLOAD = 4096


def reject_constant(value):
    raise ValueError("Non-finite JSON number")


def finite_float(value):
    result = float(value)
    if not math.isfinite(result):
        raise ValueError("Non-finite JSON number")
    return result


class DashboardState:
    """One bounded latest snapshot; MQTT and HTTP threads share a short lock."""

    def __init__(self, broker, port, root):
        self.lock = threading.Lock()
        self.broker, self.port, self.root = broker, port, root
        self.connected = False
        self.connection_note = "Connecting to broker"
        self.telemetry = None
        self.telemetry_at = None
        self.received_at = None
        self.heartbeat_at = None
        self.session_telemetry = False
        self.presence = None
        self.mapping = None
        self.histories = {}
        self.messages = 0
        self.rejected = 0

    def connection(self, connected, note):
        with self.lock:
            self.connected = connected
            self.connection_note = note
            if not connected:
                self.session_telemetry = False
                self.heartbeat_at = None

    def ingest(self, topic, raw, retained=False):
        suffix = topic.removeprefix(self.root + "/")
        if topic != self.root + "/" + suffix or suffix not in TOPICS:
            return
        try:
            if not raw or len(raw) > MAX_PAYLOAD:
                raise ValueError("Payload size")
            payload = json.loads(raw, parse_constant=reject_constant, parse_float=finite_float)
            if not isinstance(payload, dict):
                raise ValueError("Expected object")
            # Retained presence is useful context, never a live reading.
            if retained and suffix != "status":
                return
            now = time.monotonic()
            with self.lock:
                if suffix == "status" and isinstance(payload.get("online"), bool):
                    self.presence = payload["online"]
                if suffix in ("telemetry", "status") and not retained and \
                        payload.get("schema") == 3 and \
                        isinstance(payload.get("mission"), dict):
                    self.telemetry = payload
                    self.telemetry_at = now
                    self.received_at = time.time()
                    self.presence = True
                    self.session_telemetry = True
                elif suffix == "heartbeat" and payload.get("online") is True:
                    self.heartbeat_at = now
                    self.presence = True
                elif suffix == "mapping_status" and not retained:
                    self.mapping = payload
                elif suffix == "events" and not retained and \
                        payload.get("schema") == 3 and \
                        payload.get("kind") in ("hump", "barcode") and \
                        isinstance(payload.get("events"), list) and \
                        len(payload["events"]) <= 8:
                    self.histories[payload["kind"]] = payload
                elif suffix != "status":
                    self.rejected += 1
                    return
                self.messages += 1
        except (ValueError, UnicodeDecodeError, RecursionError):
            with self.lock:
                self.rejected += 1

    def snapshot(self):
        with self.lock:
            now = time.monotonic()
            elapsed = None if self.telemetry_at is None else now - self.telemetry_at
            heartbeat = None if self.heartbeat_at is None else now - self.heartbeat_at
            if not self.connected or self.presence is False:
                robot = "offline"
            elif elapsed is None or not self.session_telemetry:
                robot = "waiting"
            elif elapsed > 3 or heartbeat is None or heartbeat > 3:
                robot = "stale"
            else:
                robot = "live"
            return {
                "broker": self.broker, "port": self.port, "root": self.root,
                "test_feed": self.root.startswith("robot/dashboard-check/"),
                "broker_connected": self.connected, "connection_note": self.connection_note,
                "robot_status": robot, "received_at": self.received_at,
                "elapsed_ms": None if elapsed is None else round(elapsed * 1000),
                "heartbeat_age_ms": None if heartbeat is None else round(heartbeat * 1000),
                "messages": self.messages, "rejected": self.rejected,
                "telemetry": self.telemetry, "mapping": self.mapping,
                "histories": dict(self.histories),
            }


def make_client(state):
    import paho.mqtt.client as mqtt

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.connect_timeout = 5.0
    client.reconnect_delay_set(min_delay=1, max_delay=30)
    username = os.getenv("ROBOT_MQTT_USERNAME")
    if username:
        client.username_pw_set(username, os.getenv("ROBOT_MQTT_PASSWORD", ""))

    def on_connect(client, userdata, flags, reason_code, properties):
        if reason_code.is_failure:
            state.connection(False, "Broker refused the connection")
            return
        result, _ = client.subscribe([(state.root + "/" + t, 1) for t in TOPICS])
        if result != mqtt.MQTT_ERR_SUCCESS:
            state.connection(False, "Could not subscribe to robot topics")

    def on_subscribe(client, userdata, mid, reason_codes, properties):
        accepted = bool(reason_codes) and not any(c.is_failure for c in reason_codes)
        state.connection(accepted, "Connected" if accepted else "Broker refused subscription")

    def on_disconnect(client, userdata, flags, reason_code, properties):
        state.connection(False, "Disconnected; reconnecting automatically")

    client.on_connect = on_connect
    client.on_subscribe = on_subscribe
    client.on_connect_fail = lambda *args: state.connection(False, "Broker unavailable; retrying")
    client.on_disconnect = on_disconnect
    client.on_message = lambda client, userdata, message: state.ingest(
        message.topic, message.payload, message.retain)
    return client


class DashboardHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        path = urlsplit(self.path).path
        if path == "/api/state":
            data = json.dumps(self.server.state.snapshot(), allow_nan=False).encode()
            content_type = "application/json; charset=utf-8"
        else:
            routes = {"/": ("index.html", "text/html"),
                      "/style.css": ("style.css", "text/css"),
                      "/app.js": ("app.js", "text/javascript")}
            if path not in routes:
                self.send_error(404)
                return
            name, content_type = routes[path]
            data = (ASSETS / name).read_bytes()
            content_type += "; charset=utf-8"
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("Content-Security-Policy", "default-src 'self'; script-src 'self'; "
                         "style-src 'self'; connect-src 'self'; img-src 'self'; "
                         "object-src 'none'; base-uri 'none'; frame-ancestors 'none'")
        self.end_headers()
        try:
            self.wfile.write(data)
        except (BrokenPipeError, ConnectionResetError):
            pass

    def log_message(self, *args):
        pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--broker", default="127.0.0.1", help="Default: Mosquitto on this Mac")
    parser.add_argument("--port", type=int, default=1883, help="MQTT port")
    parser.add_argument("--root", default="robot/s3", help="Robot topic root")
    parser.add_argument("--web-port", type=int, default=8080, help="Website port; 0 chooses a free port")
    args = parser.parse_args()
    if not args.broker or not 1 <= args.port <= 65535 or not 0 <= args.web_port <= 65535:
        parser.error("Use a broker host, MQTT port 1..65535 and website port 0..65535")
    if not args.root or len(args.root.encode()) > 80 or any(
            c in args.root for c in "+#\0\r\n") or args.root.endswith("/"):
        parser.error("Use a topic root up to 80 bytes without wildcards or a trailing slash")
    state = DashboardState(args.broker, args.port, args.root)
    try:
        client = make_client(state)
    except ImportError:
        parser.exit(1, "Install: python3 -m pip install -r scripts/requirements-mqtt.txt\n")
    try:
        server = ThreadingHTTPServer(("127.0.0.1", args.web_port), DashboardHandler)
    except OSError as error:
        hint = ". Try --web-port 8081" if error.errno == errno.EADDRINUSE else ""
        parser.exit(1, f"Could not open website: {error}{hint}\n")
    server.state = state
    client.connect_async(args.broker, args.port, keepalive=15)
    client.loop_start()
    print(f"Dashboard: http://127.0.0.1:{server.server_port}", flush=True)
    print(f"MQTT: {args.broker}:{args.port} / {args.root} (display only)", flush=True)
    print("Keep this terminal open. Ctrl+C closes the dashboard.", flush=True)
    try:
        server.serve_forever(poll_interval=0.25)
    except KeyboardInterrupt:
        pass
    finally:
        client.disconnect()
        client.loop_stop()
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

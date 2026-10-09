"""Display robot MQTT observations at http://127.0.0.1:8765. Python 3.9+."""
import argparse
import json
import math
import os
from pathlib import Path
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlsplit


TELEMETRY_MAX = 1536
STATUS_MAX = 256
MAX_VIEWERS = 8
ASSET_ROOT = Path(__file__).resolve().parent.parent / "dashboard"


def uint32(value):
    return type(value) is int and 0 <= value <= 0xFFFFFFFF


def reject_constant(value):
    raise ValueError("Non-finite JSON number")


def finite_float(value):
    number = float(value)
    if not math.isfinite(number):
        raise ValueError("Non-finite JSON number")
    return number


class Observations:
    """One latest value per topic. Browser writes never run in MQTT callbacks."""

    def __init__(self):
        self.changed = threading.Condition()
        self.version = 0
        self.broker_connected = False
        self.messages = {}
        self.viewers = 0

    def connection(self, connected):
        with self.changed:
            self.broker_connected = connected
            self.version += 1
            self.changed.notify_all()

    def message(self, kind, payload, retained):
        with self.changed:
            self.messages[kind] = (payload, time.monotonic(), retained)
            self.version += 1
            self.changed.notify_all()

    def viewer(self, entering):
        with self.changed:
            if entering and self.viewers >= MAX_VIEWERS:
                return False
            self.viewers += 1 if entering else -1
            return True

    def snapshot(self, after=None):
        with self.changed:
            if after == self.version:
                self.changed.wait(timeout=1.0)
            now = time.monotonic()
            result = {"broker_connected": self.broker_connected}
            for kind in ("telemetry", "heartbeat", "status"):
                message = self.messages.get(kind)
                result[kind] = message[0] if message else None
                result[kind + "_age_ms"] = (
                    max(0, int((now - message[1]) * 1000)) if message else None
                )
            message = self.messages.get("status")
            result["status_retained"] = message[2] if message else False
            return self.version, result


def make_handler(observations):
    assets = {
        "/": ("index.html", "text/html; charset=utf-8"),
        "/index.html": ("index.html", "text/html; charset=utf-8"),
        "/dashboard.css": ("dashboard.css", "text/css; charset=utf-8"),
        "/dashboard.js": ("dashboard.js", "text/javascript; charset=utf-8"),
    }

    class DashboardHandler(BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"

        def log_message(self, format, *args):
            pass

        def do_GET(self):
            path = urlsplit(self.path).path
            if path == "/events":
                self.events()
                return
            asset = assets.get(path)
            if asset is None:
                self.send_error(404)
                return
            try:
                content = (ASSET_ROOT / asset[0]).read_bytes()
            except OSError:
                self.send_error(503, "Dashboard asset unavailable")
                return
            self.send_response(200)
            self.send_header("Content-Type", asset[1])
            self.send_header("Content-Length", str(len(content)))
            self.send_header("Cache-Control", "no-store")
            self.send_header("X-Content-Type-Options", "nosniff")
            self.end_headers()
            self.wfile.write(content)

        def events(self):
            if not observations.viewer(True):
                self.send_error(503, "Too many dashboard viewers")
                return
            try:
                self.connection.settimeout(5.0)
                self.send_response(200)
                self.send_header("Content-Type", "text/event-stream")
                self.send_header("Cache-Control", "no-store")
                self.send_header("X-Content-Type-Options", "nosniff")
                self.end_headers()
                self.wfile.write(b"retry: 1000\n\n")
                self.wfile.flush()
                version = None
                while True:
                    version, state = observations.snapshot(version)
                    data = json.dumps(state, separators=(",", ":"), allow_nan=False)
                    self.wfile.write(("event: snapshot\ndata: " + data + "\n\n").encode())
                    self.wfile.flush()
            except (OSError, ValueError):
                pass
            finally:
                observations.viewer(False)
                self.close_connection = True

    return DashboardHandler


def configure_mqtt(mqtt, args, observations):
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2,
                         protocol=mqtt.MQTTv311, clean_session=True)
    client.connect_timeout = 5.0
    client.reconnect_delay_set(min_delay=1, max_delay=30)
    username = os.getenv("ROBOT_MQTT_USERNAME")
    if username:
        client.username_pw_set(username, os.getenv("ROBOT_MQTT_PASSWORD", ""))
    topics = {args.root + "/" + kind: kind
              for kind in ("telemetry", "heartbeat", "status")}

    def on_connect(client, userdata, flags, reason_code, properties):
        if reason_code.is_failure:
            observations.connection(False)
            return
        result, _ = client.subscribe([(topic, 1) for topic in topics])
        if result != mqtt.MQTT_ERR_SUCCESS:
            observations.connection(False)

    def on_subscribe(client, userdata, mid, reason_codes, properties):
        observations.connection(not any(code.is_failure for code in reason_codes))

    def on_connect_fail(client, userdata):
        observations.connection(False)

    def on_disconnect(client, userdata, flags, reason_code, properties):
        observations.connection(False)

    def on_message(client, userdata, message):
        kind = topics.get(message.topic)
        limit = TELEMETRY_MAX if kind == "telemetry" else STATUS_MAX
        if kind is None or not message.payload or len(message.payload) > limit:
            return
        # Current telemetry/heartbeat topics are never retained. Cached data
        # from an older publisher must not establish a new live observation.
        if message.retain and kind != "status":
            return
        try:
            payload = json.loads(message.payload.decode("utf-8"),
                                 parse_constant=reject_constant,
                                 parse_float=finite_float)
        except (ValueError, UnicodeDecodeError):
            return
        if not isinstance(payload, dict) or type(payload.get("schema")) is not int:
            return
        if payload["schema"] != 1:
            return
        if kind == "telemetry":
            if not uint32(payload.get("sequence")) or not isinstance(payload.get("state"), str):
                return
        elif type(payload.get("online")) is not bool:
            return
        elif kind == "heartbeat" and (
                payload["online"] is not True or not uint32(payload.get("uptime_ms"))):
            return
        observations.message(kind, payload, bool(message.retain))

    client.on_connect = on_connect
    client.on_subscribe = on_subscribe
    client.on_connect_fail = on_connect_fail
    client.on_disconnect = on_disconnect
    client.on_message = on_message
    return client


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--broker", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=1883, help="MQTT TCP port")
    parser.add_argument("--root", default="robot/s3")
    parser.add_argument("--http-port", type=int, default=8765)
    args = parser.parse_args()
    if not 1 <= args.port <= 65535 or not 1 <= args.http_port <= 65535:
        parser.error("Ports must be 1..65535")
    if not args.root or any(c in args.root for c in "+#\0") or len(args.root.encode()) > 80:
        parser.error("Root must be 1..80 bytes without MQTT wildcards or NUL")
    try:
        import paho.mqtt.client as mqtt
    except ImportError:
        parser.exit(1, "Install: python -m pip install -r scripts/requirements-mqtt.txt\n")
    observations = Observations()
    client = configure_mqtt(mqtt, args, observations)
    try:
        server = ThreadingHTTPServer(("127.0.0.1", args.http_port),
                                     make_handler(observations))
    except OSError:
        parser.exit(1, "Could not open localhost HTTP port; check --http-port.\n")
    server.daemon_threads = True
    client.connect_async(args.broker, args.port, keepalive=15)
    # Paho's threaded loop retries both the first connect and later failures.
    if client.loop_start() != mqtt.MQTT_ERR_SUCCESS:
        server.server_close()
        parser.exit(1, "Could not start MQTT network loop.\n")
    print(f"Dashboard: http://127.0.0.1:{args.http_port}", flush=True)
    print("Display only. Waiting for robot observations; Ctrl+C to exit.", flush=True)
    try:
        server.serve_forever(poll_interval=0.25)
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
        client.disconnect()
        client.loop_stop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

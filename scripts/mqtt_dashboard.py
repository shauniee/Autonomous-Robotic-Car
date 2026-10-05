"""Local live sensor dashboard. Uses the installed Mosquitto subscriber, no pip dependencies."""
import argparse
import json
import secrets
import re
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import shutil
import subprocess
import threading
import time


def calibration_payload(action, heartbeat, telemetry, request_id):
    if action not in ("calibrate_black", "calibrate_white"):
        raise ValueError("Unknown calibration action")
    if telemetry.get("mode") != "right_ir_follow" or telemetry.get("state") not in ("READY", "STOPPED") or telemetry.get("motors", {}).get("enabled") is not False:
        raise ValueError("Calibration requires the line-following firmware with motors stopped")
    session = heartbeat.get("session")
    if heartbeat.get("online") is not True or not isinstance(session, str) or not re.fullmatch(r"[0-9a-f]{16}", session):
        raise ValueError("No valid online MQTT session")
    uptime = heartbeat.get("uptime_ms")
    if type(uptime) is not int or not 0 <= uptime <= 0xffffffff or not 0 < request_id <= 0xffffffff:
        raise ValueError("Invalid command timestamp or ID")
    return json.dumps({"session": session, "id": request_id, "issued_ms": uptime, "command": action}, separators=(",", ":"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--broker", default="127.0.0.1")
    parser.add_argument("--mqtt-port", type=int, default=1883)
    parser.add_argument("--root", default="robot/car01")
    parser.add_argument("--port", type=int, default=8765)
    args = parser.parse_args()
    executable = shutil.which("mosquitto_sub")
    if not executable:
        candidate = Path("/opt/homebrew/bin/mosquitto_sub")
        if candidate.exists():
            executable = str(candidate)
        else:
            parser.exit(1, "Install Mosquitto, which supplies mosquitto_sub.\n")
    publisher = shutil.which("mosquitto_pub") or str(Path(executable).with_name("mosquitto_pub"))
    api_token = secrets.token_hex(24)
    send_lock = threading.Lock()
    sent = {"session": None, "id": 0}
    lock = threading.Lock()
    snapshot = {"telemetry": None, "last_received": None, "online": None, "heartbeat": None, "heartbeat_received": None, "error": None}
    command = [executable, "-h", args.broker, "-p", str(args.mqtt_port), "-t", args.root + "/#", "-F", "%t\t%p"]
    subscriber = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, bufsize=1)
    def receive():
        for line in subscriber.stdout:
            topic, separator, payload = line.rstrip("\n").partition("\t")
            if not separator:
                continue
            try:
                data = json.loads(payload)
                if not isinstance(data, dict):
                    continue
            except ValueError:
                continue
            with lock:
                if topic == args.root + "/telemetry":
                    snapshot["telemetry"] = data
                    snapshot["last_received"] = time.monotonic()
                elif topic == args.root + "/status":
                    snapshot["online"] = data.get("online")
                elif topic == args.root + "/heartbeat":
                    snapshot["heartbeat"] = data
                    snapshot["heartbeat_received"] = time.monotonic()
                    snapshot["online"] = data.get("online")
        with lock:
            snapshot["error"] = "MQTT subscriber stopped; check broker and restart dashboard."
    def errors():
        for line in subscriber.stderr:
            with lock:
                snapshot["error"] = line.strip()
    threading.Thread(target=receive, daemon=True).start()
    threading.Thread(target=errors, daemon=True).start()
    page_path = Path(__file__).resolve().parent.parent / "dashboard" / "index.html"
    class Handler(BaseHTTPRequestHandler):
        def do_GET(self):
            if self.path == "/api":
                with lock:
                    data = dict(snapshot)
                received = data.pop("last_received")
                data["age_s"] = None if received is None else round(time.monotonic() - received, 2)
                data["api_token"] = api_token
                data["live"] = received is not None and data["age_s"] < 3 and data["online"] is not False
                body = json.dumps(data, allow_nan=False).encode()
                mime = "application/json"
            elif self.path in ("/", "/index.html"):
                body, mime = page_path.read_bytes(), "text/html; charset=utf-8"
            else:
                self.send_error(404)
                return
            self.send_response(200)
            self.send_header("Content-Type", mime)
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        def do_POST(self):
            status, result = 200, {}
            try:
                if self.path != "/calibrate" or self.headers.get("Content-Type") != "application/json":
                    raise ValueError("Unknown endpoint or content type")
                length = int(self.headers.get("Content-Length", "0"))
                if not 0 < length <= 512:
                    raise ValueError("Invalid request size")
                request = json.loads(self.rfile.read(length))
                if not isinstance(request, dict):
                    raise ValueError("Invalid request")
                if not secrets.compare_digest(str(request.get("token", "")), api_token):
                    raise ValueError("Reload the dashboard before calibrating")
                with send_lock:
                    with lock:
                        current = dict(snapshot)
                    now = time.monotonic()
                    if current["last_received"] is None or now - current["last_received"] > 3 or current["heartbeat_received"] is None or now - current["heartbeat_received"] > 2:
                        raise ValueError("No fresh robot telemetry/heartbeat")
                    heartbeat = current["heartbeat"]
                    session = heartbeat.get("session")
                    previous = sent["id"] if sent["session"] == session else 0
                    last_id = heartbeat.get("last_command_id")
                    if type(last_id) is not int or not 0 <= last_id < 0xffffffff:
                        raise ValueError("Invalid command counter")
                    request_id = max(previous, last_id) + 1
                    payload = calibration_payload(request.get("action"), heartbeat, current["telemetry"], request_id)
                    subprocess.run([publisher, "-h", args.broker, "-p", str(args.mqtt_port), "-t", args.root + "/command", "-m", payload, "-q", "1"], check=True, capture_output=True, timeout=5)
                    sent.update(session=session, id=request_id)
                    result = {"request_id": request_id, "message": "Sent via MQTT; waiting for robot acknowledgement"}
            except (ValueError, TypeError, subprocess.SubprocessError) as error:
                status, result = 400, {"error": str(error)}
            body = json.dumps(result).encode()
            self.send_response(status)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        def log_message(self, *args):
            pass
    try:
        with ThreadingHTTPServer(("127.0.0.1", args.port), Handler) as server:
            print(f"Dashboard: http://127.0.0.1:{args.port}  MQTT: {args.broker}:{args.mqtt_port}", flush=True)
            server.serve_forever()
    except KeyboardInterrupt:
        return 0
    finally:
        subscriber.terminate()
        subscriber.wait(timeout=5)


if __name__ == "__main__":
    raise SystemExit(main())

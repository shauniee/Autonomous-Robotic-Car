"""Local live sensor dashboard. Uses the installed Mosquitto subscriber, no pip dependencies."""
import argparse
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import shutil
import subprocess
import threading
import time


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
    lock = threading.Lock()
    snapshot = {"telemetry": None, "last_received": None, "online": None, "heartbeat": None, "error": None}
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
                    snapshot["online"] = data.get("online")
        with lock:
            snapshot["error"] = "MQTT subscriber stopped; check broker and restart dashboard."
    def errors():
        for line in subscriber.stderr:
            with lock:
                snapshot["error"] = line.strip()
    threading.Thread(target=receive, daemon=True).start()
    threading.Thread(target=errors, daemon=True).start()
    page = (Path(__file__).resolve().parent.parent / "dashboard" / "index.html").read_bytes()
    class Handler(BaseHTTPRequestHandler):
        def do_GET(self):
            if self.path == "/api":
                with lock:
                    data = dict(snapshot)
                received = data.pop("last_received")
                data["age_s"] = None if received is None else round(time.monotonic() - received, 2)
                data["live"] = received is not None and data["age_s"] < 3 and data["online"] is not False
                body = json.dumps(data, allow_nan=False).encode()
                mime = "application/json"
            elif self.path in ("/", "/index.html"):
                body, mime = page, "text/html; charset=utf-8"
            else:
                self.send_error(404)
                return
            self.send_response(200)
            self.send_header("Content-Type", mime)
            self.send_header("Cache-Control", "no-store")
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

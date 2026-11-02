"""INT-68 synthetic UI fixtures; no MQTT connection or robot commands."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[4]
FRAME = dict(type="telemetry", sequence=142, mission="FOLLOW_LINE", motion="RUNNING",
             action_id=12, line="VISIBLE", faults=[], mapped=True, left=6534,
             right=10252, speed_left=118, speed_right=121, direction="straight",
             barcode="A", barcode_action="LEFT", front_mm=341,
             obstacle_width_mm=142, obstacle_depth_mm=216, hump_count=2, hump_max_mm=24)

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/events":
            self.send_response(200)
            self.send_header("Content-Type", "text/event-stream")
            self.end_headers()
            return
        frame = FRAME.copy()
        route = self.path.strip("/")
        if route in ("left", "right", "reverse"):
            frame["direction"] = route
        if route == "fault":
            frame.update(mission="STOPPED", motion="FAILED", direction="stopped",
                         speed_left=0, speed_right=0,
                         faults=["Line: IR invalid", "Terrain: IMU calibration failed", "Mission: line lost"])
        if route == "invalid":
            frame.update(direction=None, speed_left=None, speed_right=None)
        script = "source.close(); lamp('link', 'Synthetic UI fixture', 'warn');"
        script += "lamp('robot', 'No robot connected', 'warn');"
        if route != "empty":
            script += "setupStep = 2; showSetup(); showTelemetry(" + json.dumps(frame) + ");"
            if route == "stale":
                script += "lastFrame = Date.now() - 5000; document.body.classList.add('stale'); gate(); sensorFreshness();"
            else:
                script += "setInterval(() => showTelemetry(" + json.dumps(frame) + "), 500);"
        page = (ROOT / "scripts/dashboard.html").read_text()
        if route == "reduced":
            # Activate the production reduced-motion rules to verify their cascade.
            page = page.replace("@media (prefers-reduced-motion: reduce)", "@media all")
        body = page.replace("</body>", "<script>" + script + "</script></body>").encode()
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *args):
        pass

ThreadingHTTPServer(("127.0.0.1", 8090), Handler).serve_forever()

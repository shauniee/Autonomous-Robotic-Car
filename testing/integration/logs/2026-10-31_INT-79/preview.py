"""INT-79 synthetic calibration display; no MQTT or robot commands."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[4]
FRAME = dict(sequence=1, mission="READY", motion="IDLE", action_id=0,
             line="VISIBLE", faults=[], mapped=False, left=0, right=0,
             speed_left=0, speed_right=0, direction="stopped", barcode="none",
             barcode_action="UNMAPPED", front_mm=None, obstacle_width_mm=0,
             obstacle_depth_mm=0, hump_count=0, hump_max_mm=None)

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/events":
            self.send_response(200)
            self.send_header("Content-Type", "text/event-stream")
            self.end_headers()
            return
        frame = dict(FRAME, imu_calibrated=True)
        if self.path == "/uncalibrated":
            frame["imu_calibrated"] = False
        elif self.path == "/legacy":
            frame.pop("imu_calibrated")
        elif self.path == "/fault":
            frame["faults"] = ["Terrain: IMU"]
        script = "source.close();lamp('link','Synthetic UI fixture','warn');"
        script += "lamp('robot','No robot connected','warn');"
        script += "setInterval(() => showTelemetry(" + json.dumps(frame) + "),500);"
        page = (ROOT / "scripts/dashboard.html").read_text()
        body = page.replace("</body>", "<script>" + script + "</script></body>").encode()
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *args):
        pass

if __name__ == "__main__":
    ThreadingHTTPServer(("127.0.0.1", 8091), Handler).serve_forever()

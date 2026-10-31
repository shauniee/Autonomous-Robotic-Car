import http.server
import json
import time
from pathlib import Path
page = Path('scripts/dashboard.html').read_text()
frame = dict(type='telemetry', sequence=142, mission='FOLLOW_LINE', motion='RUNNING', action_id=12, line='VISIBLE', faults=[], mapped=True, left=6534, right=10252, speed_left=118, speed_right=121, direction='straight', barcode='A', barcode_action='LEFT', front_mm=341, obstacle_width_mm=142, obstacle_depth_mm=216, hump_count=2, hump_max_mm=24)
class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/events':
            self.send_response(200)
            self.send_header('Content-Type', 'text/event-stream')
            self.end_headers()
            try:
                while True:
                    self.wfile.write(('data: ' + json.dumps(frame) + '\n\n').encode())
                    self.wfile.flush()
                    time.sleep(.5)
            except (BrokenPipeError, ConnectionResetError):
                pass
        else:
            # Separate fixtures leave the production event stream untouched.
            script = '''<script>source.close(); lastFrame = 0; document.body.classList.add('stale');
            lamp('link', 'Fixture preview', 'warn'); lamp('robot', 'Synthetic data', 'warn');</script>'''
            if self.path != '/empty':
                f = frame.copy()
                if self.path == '/invalid':
                    f.update(front_mm=None, hump_max_mm=None, speed_left=None, speed_right=None, barcode='-', obstacle_width_mm=0, obstacle_depth_mm=0)
                script = script.replace('</script>', 'showTelemetry(' + json.dumps(f) + ');' + ('' if self.path == '/stale' else 'setInterval(() => showTelemetry(' + json.dumps(f) + '), 500);') + '</script>')
                if self.path == '/stale':
                    script += "<script>lastFrame = Date.now()-5000; sensorFreshness(); document.body.classList.add('stale');</script>"
            body = page.replace('</body>', script + '</body>').encode()
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.end_headers()
            self.wfile.write(body)
    def log_message(self, *args): pass
http.server.ThreadingHTTPServer(('127.0.0.1', 8081), Handler).serve_forever()

"""Synthetic, loopback-only UI fixtures; never connects to a robot or broker."""
import http.server
import json
from pathlib import Path
from urllib.parse import urlparse, parse_qs

FRAME = dict(type='telemetry', sequence=142, mission='FOLLOW_LINE', motion='RUNNING', action_id=12, line='VISIBLE', faults=[], mapped=True, left=6534, right=10252, speed_left=118, speed_right=121, direction='straight', barcode='A', barcode_action='LEFT', front_mm=341, obstacle_width_mm=142, obstacle_depth_mm=216, hump_count=2, hump_max_mm=24)

class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/events':
            self.send_response(204)
            self.end_headers()
            return
        query = parse_qs(urlparse(self.path).query)
        mode = query.get('mode', ['live'])[0]
        frame = dict(FRAME, mission=query.get('mission', ['FOLLOW_LINE'])[0])
        if mode == 'fault':
            frame.update(mission='STOPPED', motion='FAILED', line='LOST', front_mm=None,
                         faults=['Motion: motor stall', 'Line: barcode capture overflow', 'Terrain: IMU calibration failed', 'Obstacle: no echo', 'Mission: line lost', 'Network: broker lost'])
        if mode == 'invalid':
            frame.update(front_mm=None, hump_max_mm=None, speed_left=None, speed_right=None, barcode='-', mapped=False)
        script = "document.getElementById('map').addEventListener('submit', () => { if (pending) setTimeout(() => showAck({sequence:17,ok:true,text:'Synthetic mapping ACK OK'}),100); }); source.close(); lamp('link', 'Fixture preview', 'warn'); lamp('robot', 'Synthetic data', 'warn'); robotOnline = true;"
        if mode != 'empty':
            script += 'showTelemetry(' + json.dumps(frame) + ');'
            if mode == 'stale':
                script += "lastFrame = Date.now() - 5000; sensorFreshness(); document.body.classList.add('stale'); gate();"
            else:
                script += 'setInterval(() => showTelemetry(' + json.dumps(frame) + '), 500);'
            script += "addEvent('Illustrative barcode A → LEFT'); addEvent('Illustrative hump: 24 mm');"
        body = Path('scripts/dashboard.html').read_text().replace('</body>', '<script>' + script + '</script></body>').encode()
        self.send_response(200)
        self.send_header('Content-Type', 'text/html; charset=utf-8')
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self):
        # A synthetic ACK exercises UI behaviour without MQTT or motion.
        self.rfile.read(int(self.headers.get('Content-Length', 0)))
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(b'{"sequence": 17}')

    def log_message(self, *args):
        pass

http.server.ThreadingHTTPServer(('127.0.0.1', 8082), Handler).serve_forever()

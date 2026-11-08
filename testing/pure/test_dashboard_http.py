"""Host-only dashboard HTTP validation; no MQTT broker or robot connected."""
import sys
import json
from pathlib import Path
from urllib.request import Request, urlopen
from urllib.error import HTTPError

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
import mqtt_client as client
dashboard = client.Dashboard(0)
port = dashboard.server.server_port
origin = f'http://127.0.0.1:{port}'
sent = []
def send(action):
    sent.append(action)
    return 7
dashboard.send_direction = send

def post(path, body, source=origin):
    request = Request(origin + path, data=json.dumps(body).encode(),
                      headers={'Content-Type': 'application/json', 'Origin': source})
    try:
        with urlopen(request) as response:
            return response.status, json.load(response)
    except HTTPError as error:
        return error.code, None

try:
    for action in range(1, 5):
        status, reply = post('/direction', {'action': action})
        assert status == 200 and reply['sequence'] == 7
    assert sent == [1, 2, 3, 4]
    for action in [0, 5, -1, True, '1', None]:
        assert post('/direction', {'action': action})[0] == 400
    assert post('/direction', {'action': 1}, 'http://foreign.example')[0] == 403
    assert post('/unknown', {'action': 1})[0] == 403
    dashboard.send_direction = None
    assert post('/direction', {'action': 1})[0] == 503
    dashboard.send_mapping = lambda letters: 8
    assert post('/map', {'letters': 'ABCD'}) == (200, {'sequence': 8})
    assert post('/map', {'letters': 'AAAA'})[0] == 400
    print('dashboard HTTP: PASS (four actions, invalid inputs, origin, disconnected, mapping regression)')
finally:
    dashboard.server.shutdown()
    dashboard.server.server_close()

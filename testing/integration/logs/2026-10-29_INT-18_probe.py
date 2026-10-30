"""Run inside dashboard with stdin; synthetic transport evidence only."""
import importlib.util
import json
import threading
import urllib.request
import urllib.error
import paho.mqtt.client as mqtt

spec = importlib.util.spec_from_file_location('client', '/app/mqtt_client.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
assert module.self_test() == 0
base = 'http://127.0.0.1:8080'
assert urllib.request.urlopen(base, timeout=5).status == 200
for origin, expected in [('http://evil.example', 403), (base, 400)]:
    request = urllib.request.Request(base + '/map', data=b'{"letters":"AAAA"}',
        headers={'Content-Type': 'application/json', 'Origin': origin})
    try:
        urllib.request.urlopen(request, timeout=5)
        raise AssertionError('Invalid mapping unexpectedly accepted')
    except urllib.error.HTTPError as error:
        assert error.code == expected, error.code
print('HTTP and mapping-origin checks: PASS', flush=True)
received = threading.Event()
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
def connected(client, userdata, flags, reason, properties):
    client.subscribe('testing/INT-18', qos=1)
def subscribed(client, userdata, mid, reasons, properties):
    client.publish('testing/INT-18', b'transport-only', qos=1)
def message(client, userdata, message):
    if message.payload == b'transport-only':
        received.set()
client.on_connect = connected
client.on_subscribe = subscribed
client.on_message = message
client.connect('mosquitto', 1883)
client.loop_start()
assert received.wait(10), 'MQTT round trip timed out'
with urllib.request.urlopen(base + '/events', timeout=20) as response:
    # Synthetic invalid frame exercises dashboard subscription without robot commands.
    client.publish('robot/s3/telemetry', b'INT-18-synthetic', qos=1).wait_for_publish(5)
    for _ in range(20):
        line = response.readline()
        if b'undecodable frame' in line:
            break
    else:
        raise AssertionError('Dashboard did not receive MQTT frame')
client.disconnect()
client.loop_stop()
print('MQTT round trip and dashboard stream: PASS (synthetic, no robot)', flush=True)

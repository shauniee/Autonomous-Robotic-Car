"""CLI exit status against in-process MQTT replies; no network or robot."""
import importlib.util
from pathlib import Path
import sys
from types import ModuleType, SimpleNamespace
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("mqtt_client",
    Path(__file__).resolve().parents[2] / "scripts/mqtt_client.py")
cli = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cli)


class Client:
    ack_result = 0

    def __init__(self, *args):
        pass

    def connect(self, *args, **kwargs):
        self.on_connect(self, None, None, SimpleNamespace(is_failure=False), None)

    def subscribe(self, *args):
        pass

    def loop_start(self):
        pass

    def publish(self, topic, payload, **kwargs):
        if cli.frame_type(payload) == cli.SNAPSHOT_REQUEST:
            reply = bytes([0x23]) + cli.GOLDEN_TELEMETRY[1:]
        else:
            # An unrelated OK ACK must not override our rejected request.
            self.on_message(self, None, SimpleNamespace(topic="robot/s3/telemetry",
                payload=bytes([0x25, (payload[1] + 1) & 255, 0])))
            reply = bytes([0x25, payload[1], self.ack_result])
        self.on_message(self, None, SimpleNamespace(topic="robot/s3/telemetry",
            payload=reply))
        return SimpleNamespace(wait_for_publish=lambda **kwargs: None)

    def disconnect(self):
        pass

    def loop_stop(self):
        pass


paho = ModuleType("paho")
mqtt = ModuleType("paho.mqtt")
client = ModuleType("paho.mqtt.client")
client.Client = Client
client.CallbackAPIVersion = SimpleNamespace(VERSION2=2)
paho.mqtt = mqtt
mqtt.client = client
modules = {"paho": paho, "paho.mqtt": mqtt, "paho.mqtt.client": client}
with patch.dict(sys.modules, modules), patch.dict(cli.os.environ, {}, clear=True):
    with patch.object(sys, "argv", ["mqtt_client.py", "map", "A", "B", "C", "D", "--broker", "unused"]):
        for result in range(5):
            Client.ack_result = result
            assert cli.main() == (0 if result == 0 else 1)
    with patch.object(sys, "argv", ["mqtt_client.py", "snapshot", "--broker", "unused"]):
        assert cli.main() == 0
print("mqtt CLI: PASS (OK/rejected/unknown ACKs, sequence matching, snapshot)")

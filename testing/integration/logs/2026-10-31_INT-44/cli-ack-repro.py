"""Exercise the real CLI with an in-process MQTT test double; no network."""
import importlib.util
from pathlib import Path
import sys
from types import ModuleType, SimpleNamespace
from unittest.mock import patch

root = Path(__file__).resolve().parents[4]
spec = importlib.util.spec_from_file_location("mqtt_client", root / "scripts/mqtt_client.py")
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
        self.on_message(self, None, SimpleNamespace(topic="robot/s3/telemetry",
            payload=bytes([0x25, payload[1], self.ack_result])))
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
with patch.dict(sys.modules, modules), patch.dict(cli.os.environ, {}, clear=True), \
        patch.object(sys, "argv", ["mqtt_client.py", "map", "A", "B", "C", "D", "--broker", "unused"]):
    for result in range(4):
        Client.ack_result = result
        code = cli.main()
        print(f"ACK result={result}, CLI exit={code}")
        assert code == 0
print("Defect reproduced: BUSY, INVALID and MALFORMED all exit successfully.")

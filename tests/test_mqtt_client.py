import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("robot_mqtt_client", ROOT / "scripts/mqtt_client.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class MqttHelperTests(unittest.TestCase):
    def test_builds_session_command_from_board_time(self):
        result = json.loads(module.start_payload({"online": True, "session": "0123456789abcdef",
                            "uptime_ms": 1000, "last_command_id": 5}))
        self.assertEqual(result, {"session": "0123456789abcdef", "id": 6,
                                 "issued_ms": 1000, "command": "start"})

    def test_rejects_invalid_heartbeats(self):
        good = {"online": True, "session": "0123456789abcdef",
                "uptime_ms": 1000, "last_command_id": 5}
        for key, value in (("online", False), ("session", "old"), ("uptime_ms", -1),
                           ("uptime_ms", True), ("last_command_id", 0xFFFFFFFF)):
            with self.subTest(key=key, value=value), self.assertRaises(ValueError):
                module.start_payload({**good, key: value})
        with self.assertRaises(ValueError):
            module.start_payload([])

    def test_c_encoder_produces_valid_json(self):
        fixture = ROOT / "build/telemetry-fixture.json"
        self.assertTrue(fixture.exists(), "Run scripts/check.ps1 first")
        data = json.loads(fixture.read_text())
        self.assertEqual(data["schema"], 1)
        self.assertEqual(data["reason"], 'quote"\\\n')
        self.assertIsNone(data["motion"]["left_mps"])
        self.assertIsNone(data["obstacle"]["front_distance_m"])


if __name__ == "__main__":
    unittest.main()

"""Defect reproduction using the actual laptop decoder; no network."""
import importlib.util
from pathlib import Path

root = Path(__file__).resolve().parents[4]
spec = importlib.util.spec_from_file_location("mqtt_client", root / "scripts/mqtt_client.py")
client = importlib.util.module_from_spec(spec)
spec.loader.exec_module(client)
frame = bytearray(client.GOLDEN_TELEMETRY)
frame[3] |= 0x80  # Reserved bit, prohibited by the schema.
frame[16:18] = (0x3fff).to_bytes(2, "little")  # Valid + invalid range sentinel.
message = client.dashboard_message(bytes(frame), client.Motion())
assert message["type"] == "telemetry" and message["front_mm"] == 8191
print("Malformed telemetry accepted: reserved bit set, front_valid+invalid sentinel displayed as", message["front_mm"], "mm")

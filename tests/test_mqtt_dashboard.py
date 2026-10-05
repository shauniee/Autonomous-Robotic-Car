"""Local integration check against the real running broker, on an isolated test topic."""
import json
from pathlib import Path
import shutil
import subprocess
import sys
import time
from urllib.request import urlopen

root = Path(__file__).resolve().parent.parent
publisher = shutil.which("mosquitto_pub") or "/opt/homebrew/bin/mosquitto_pub"
topic = "codex/sensor-bench-test/telemetry"
process = subprocess.Popen([sys.executable, "scripts/mqtt_dashboard.py", "--port", "8766", "--root", "codex/sensor-bench-test"], cwd=root, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
try:
    for attempt in range(40):
        try:
            with urlopen("http://127.0.0.1:8766/api", timeout=1) as response:
                state = json.load(response)
            break
        except OSError:
            if process.poll() is not None:
                raise RuntimeError(process.stderr.read())
            time.sleep(.1)
    else:
        raise RuntimeError("Test dashboard did not start")
    subprocess.run([publisher, "-h", "127.0.0.1", "-t", topic, "-f", str(root / "build-host/bench-fixture.json"), "-q", "1", "-r"], check=True)
    for attempt in range(40):
        with urlopen("http://127.0.0.1:8766/api", timeout=1) as response:
            state = json.load(response)
        if state["telemetry"]:
            break
        time.sleep(.1)
    assert state["live"] is True
    assert state["telemetry"]["ir_right"]["raw_adc"] == 2048
    assert state["telemetry"]["motors"]["enabled"] is False
    time.sleep(3.1)
    with urlopen("http://127.0.0.1:8766/api", timeout=1) as response:
        state = json.load(response)
    assert state["live"] is False
    print("Actual local MQTT broker -> dashboard data and stale-state check passed (isolated synthetic topic; no hardware claim).")
finally:
    subprocess.run([publisher, "-h", "127.0.0.1", "-t", topic, "-n", "-r"], check=True)
    process.send_signal(2)
    process.wait(timeout=5)

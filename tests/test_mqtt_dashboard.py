"""Local integration check against the real running broker, on an isolated test topic."""
import json
from pathlib import Path
import shutil
import subprocess
import sys
import time
from urllib.request import urlopen, Request

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
    heartbeat={"online":True,"session":"0123456789abcdef","uptime_ms":100,"last_command_id":0}
    follow={"mode":"right_ir_follow","state":"READY","motors":{"enabled":False}}
    for suffix,data in [("heartbeat",heartbeat),("telemetry",follow)]:
        subprocess.run([publisher,"-h","127.0.0.1","-t","codex/sensor-bench-test/"+suffix,"-m",json.dumps(data),"-r","-q","1"],check=True)
    time.sleep(.2)
    with urlopen("http://127.0.0.1:8766/api") as response: current=json.load(response)
    subscriber = subprocess.Popen([str(Path(publisher).with_name("mosquitto_sub")),"-h","127.0.0.1","-t","codex/sensor-bench-test/command","-C","1","-W","5"],stdout=subprocess.PIPE,text=True)
    try:
        time.sleep(.2)
        request=Request("http://127.0.0.1:8766/calibrate",data=json.dumps({"action":"calibrate_black","token":current["api_token"]}).encode(),headers={"Content-Type":"application/json"})
        with urlopen(request) as response: result=json.load(response)
        message=json.loads(subscriber.communicate(timeout=6)[0])
        assert message["command"]=="calibrate_black" and message["id"]==result["request_id"]
        assert message["session"]==heartbeat["session"]
    finally:
        if subscriber.poll() is None: subscriber.terminate(); subscriber.wait(timeout=2)
    print("Actual dashboard calibration POST -> MQTT command passed on isolated test topic.")
    time.sleep(3.1)
    with urlopen("http://127.0.0.1:8766/api", timeout=1) as response:
        state = json.load(response)
    assert state["live"] is False
    print("Actual local MQTT broker -> dashboard data and stale-state check passed (isolated synthetic topic; no hardware claim).")
finally:
    subprocess.run([publisher, "-h", "127.0.0.1", "-t", topic, "-n", "-r"], check=True)
    subprocess.run([publisher, "-h", "127.0.0.1", "-t", "codex/sensor-bench-test/heartbeat", "-n", "-r"], check=True)
    process.send_signal(2)
    process.wait(timeout=5)

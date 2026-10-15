"""Focused dashboard checks through a REAL broker and local HTTP server.

Run from code/: python3 testing/subsystem1/dashboard_check.py --broker 127.0.0.1
Uses an isolated, unique topic root; never publishes on robot/s3.
"""
import argparse
from datetime import datetime
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import threading
import time
import urllib.error
import urllib.request
import uuid
from zoneinfo import ZoneInfo

import paho.mqtt.client as mqtt


REPO = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("dashboard", REPO / "scripts/mqtt_dashboard.py")
dashboard = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dashboard)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--broker", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--preview-seconds", type=int, default=0,
                        help="Keep the isolated synthetic feed available for browser inspection")
    args = parser.parse_args()
    if not 0 <= args.preview_seconds <= 600:
        parser.error("Preview duration must be 0..600 seconds")
    if args.output and args.output.exists():
        parser.error("Evidence already exists; choose a new filename")
    source_files = [REPO / "scripts/mqtt_dashboard.py", Path(__file__),
                    *sorted((REPO / "scripts/dashboard").glob("*")),
                    Path(__file__).parent / "data/dashboard-sample.json"]
    source_hashes = {str(p.relative_to(REPO)): hashlib.sha256(p.read_bytes()).hexdigest()
                     for p in source_files if p.is_file()}
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=REPO, text=True).strip()
    root = "robot/dashboard-check/" + uuid.uuid4().hex[:12]
    state = dashboard.DashboardState(args.broker, args.port, root)
    subscriber = dashboard.make_client(state)
    publisher = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    server = dashboard.ThreadingHTTPServer(("127.0.0.1", 0), dashboard.DashboardHandler)
    server.state = state
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    url = f"http://127.0.0.1:{server.server_port}"
    sample = json.loads((Path(__file__).parent / "data/dashboard-sample.json").read_text())
    rows = []

    def fetch(path="/api/state"):
        with urllib.request.urlopen(url + path, timeout=3) as response:
            return response.status, response.read()

    def current():
        return json.loads(fetch()[1])

    def wait_for(predicate, timeout=6):
        end = time.monotonic() + timeout
        while time.monotonic() < end:
            value = current()
            if predicate(value):
                return value
            time.sleep(0.05)
        raise AssertionError("Timed out waiting for expected dashboard state")

    def publish(suffix, value, retain=False):
        wire = json.dumps(value, separators=(",", ":")) if not isinstance(value, bytes) else value
        info = publisher.publish(root + "/" + suffix, wire, qos=1, retain=retain)
        info.wait_for_publish(timeout=3)
        assert info.is_published()

    def record(test_id, description, operation):
        try:
            operation()
            rows.append({"id": test_id, "description": description, "result": "PASS"})
            print(f"[WEB] {test_id} PASS {description}", flush=True)
        except Exception as error:
            rows.append({"id": test_id, "description": description,
                         "result": "FAIL", "error": str(error)})
            raise

    try:
        publisher.connect(args.broker, args.port, keepalive=15)
        publisher.loop_start()
        publish("telemetry", sample, retain=True)
        publish("status", {"schema": 1, "online": True}, retain=True)
        subscriber.connect_async(args.broker, args.port, keepalive=15)
        subscriber.loop_start()
        wait_for(lambda s: s["broker_connected"] and s["messages"] > 0)

        def assets():
            for path in ("/", "/app.js", "/style.css"):
                assert fetch(path)[0] == 200
            for path in ("/config/wifi_secrets.h", "/../config/wifi_secrets.h"):
                try:
                    fetch(path)
                    raise AssertionError("Non-asset path was served")
                except urllib.error.HTTPError as error:
                    assert error.code == 404
        record("S1-W01", "page/assets served; repository and credential paths unavailable", assets)

        def retained():
            s = current()
            assert s["telemetry"] is None and s["robot_status"] == "waiting"
        record("S1-W02", "retained presence/telemetry cannot become a live robot snapshot", retained)

        def live():
            publish("heartbeat", {"schema": 1, "online": True})
            publish("telemetry", sample)
            s = wait_for(lambda s: s["robot_status"] == "live")
            assert s["telemetry"]["motion"]["speed_mm_s"] == [120, 116]
            assert s["telemetry"]["terrain"]["max_height_mm"] == 38
            publish("events", {"schema": 3, "kind": "hump", "run": 7, "after_id": 0,
                               "count": 2, "dropped": 0, "overflow": False,
                               "events": [sample["events"]["latest_hump"]]})
            wait_for(lambda s: "hump" in s["histories"])
        record("S1-W03", "real broker delivery updates readings and accepted event history", live)

        def malformed():
            before = current()["rejected"]
            for value in (b"{broken", b"[]", b"x" * 4097,
                          b'{"schema":3,"mission":{},"n":NaN}',
                          b'{"schema":3,"mission":{},"n":1e999}'):
                publish("telemetry", value)
            s = wait_for(lambda s: s["rejected"] >= before + 5)
            assert s["telemetry"]["sequence"] == sample["sequence"]
        record("S1-W04", "malformed/oversized/non-finite data cannot replace valid readings", malformed)

        def stale():
            s = wait_for(lambda s: s["robot_status"] == "stale", timeout=5)
            assert s["elapsed_ms"] > 3000 and s["telemetry"]["terrain"]["max_height_mm"] == 38
            publish("status", {"schema": 1, "online": False})
            wait_for(lambda s: s["robot_status"] == "offline")
        record("S1-W05", "missing heartbeat/snapshots expire; offline presence wins; accepted peak retained", stale)

        def reconnect():
            subscriber.disconnect()
            subscriber.loop_stop()
            wait_for(lambda s: not s["broker_connected"])
            subscriber.reconnect()
            subscriber.loop_start()
            s = wait_for(lambda s: s["broker_connected"])
            assert s["robot_status"] != "live"
            publish("heartbeat", {"schema": 1, "online": True})
            publish("telemetry", sample)
            wait_for(lambda s: s["robot_status"] == "live")
        record("S1-W06", "reconnected client requires fresh telemetry and heartbeat", reconnect)
        if args.preview_seconds:
            print(f"Synthetic preview: {url} ({root})", flush=True)
            for tick in range(args.preview_seconds * 2):
                sample["sequence"] += 1
                publish("heartbeat", {"schema": 1, "online": True})
                publish("telemetry", sample)
                time.sleep(0.5)
    finally:
        for suffix in ("status", "telemetry"):
            try:
                info = publisher.publish(root + "/" + suffix, b"", qos=1, retain=True)
                info.wait_for_publish(timeout=2)
            except (RuntimeError, ValueError):
                pass
        subscriber.disconnect()
        subscriber.loop_stop()
        publisher.disconnect()
        publisher.loop_stop()
        server.shutdown()
        server.server_close()
        result = {"recorded_at": datetime.now(ZoneInfo("Asia/Singapore")).isoformat(),
                  "kind": "Laptop HTTP + real MQTT broker checks; synthetic data, no Pico proof",
                  "base_revision": revision,
                  "broker": args.broker, "topic_root": root, "synthetic": True,
                  "source_sha256": source_hashes,
                  "checks": rows,
                  "result": "PASS" if len(rows) == 6 and all(r["result"] == "PASS" for r in rows) else "FAIL"}
        if args.output:
            if args.output.exists():
                raise RuntimeError("Evidence exists; choose a new filename")
            args.output.write_text(json.dumps(result, indent=2) + "\n")


if __name__ == "__main__":
    main()

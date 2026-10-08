"""Optional real Mac broker smoke check; no Pico or private-network access.

Run after scripts/check-ir-tracking.sh: python3 tests/test_mqtt_broker.py
Uses an isolated loopback port, CLI subscriber and the real C encoder fixture.
"""
import json
import os
from pathlib import Path
import selectors
import shutil
import socket
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
TOPIC = "robot/s3/telemetry"


def command(name):
    found = shutil.which(name)
    if not found:
        found = f"/opt/homebrew/{'sbin' if name == 'mosquitto' else 'bin'}/{name}"
    assert Path(found).is_file(), f"Install Mosquitto before running this optional check: {name}"
    return found


def read_until(process, predicate, timeout=7):
    # Read bytes so Python buffering cannot hide already-arrived debug lines.
    selection = selectors.DefaultSelector()
    selection.register(process.stdout, selectors.EVENT_READ)
    buffer = b""
    deadline = time.monotonic() + timeout
    try:
        while time.monotonic() < deadline:
            for _ in selection.select(max(0, deadline - time.monotonic())):
                data = os.read(process.stdout.fileno(), 1)
                assert data, "subscriber exited before expected event"
                buffer += data
                if data == b"\n":
                    line = buffer.decode().strip()
                    buffer = b""
                    if predicate(line):
                        return line
        raise AssertionError("subscriber event timeout")
    finally:
        selection.close()


def main():
    payload = subprocess.check_output([ROOT / "build-host/test_telemetry", "--sample"], text=True).strip()
    decoded = json.loads(payload)
    assert decoded["error"] == -0.7 and decoded["sample_age_ms"] == 20
    assert len(payload.encode()) <= 768 and decoded["test_inhibited"]
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]
    address = ["-h", "127.0.0.1", "-p", str(port), "-V", "mqttv311"]
    processes = []
    with tempfile.TemporaryDirectory(prefix="s3-mqtt-broker-") as directory:
        config = Path(directory) / "mosquitto.conf"
        config.write_text(f"listener {port} 127.0.0.1\nallow_anonymous true\npersistence false\n")

        def broker():
            process = subprocess.Popen([command("mosquitto"), "-c", str(config)],
                                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            processes.append(process)
            deadline = time.monotonic() + 3
            while time.monotonic() < deadline:
                assert process.poll() is None, "broker failed to start"
                try:
                    with socket.create_connection(("127.0.0.1", port), timeout=.1):
                        return process
                except OSError:
                    time.sleep(.02)
            raise AssertionError("broker startup timeout")

        def publish(message):
            subprocess.run([command("mosquitto_pub"), *address, "-q", "0", "-t", TOPIC, "-m", message],
                           check=True, timeout=5, capture_output=True)

        try:
            server = broker()
            # A PTY gives the CLI the same line-buffered output as a terminal.
            master, slave = os.openpty()
            subscriber = subprocess.Popen([command("mosquitto_sub"), *address, "-q", "0", "-t", TOPIC,
                                           "-d", "-v", "-C", "2", "-W", "15"],
                                          stdout=slave, stderr=subprocess.STDOUT)
            os.close(slave)
            subscriber.stdout = os.fdopen(master, "rb", buffering=0)
            processes.append(subscriber)
            ready = lambda line: "received SUBACK" in line
            read_until(subscriber, ready)
            publish(payload)
            received = read_until(subscriber, lambda line: line.startswith(TOPIC + " "))
            assert received == TOPIC + " " + payload
            server.terminate()
            server.wait(timeout=3)
            refused = subprocess.run([command("mosquitto_pub"), *address, "-t", TOPIC, "-m", payload],
                                     capture_output=True, timeout=5)
            assert refused.returncode != 0
            server = broker()
            read_until(subscriber, ready)
            decoded["sequence"] += 1
            latest = json.dumps(decoded, separators=(",", ":"))
            publish(latest)
            assert read_until(subscriber, lambda line: line.startswith(TOPIC + " ")) == TOPIC + " " + latest
            assert subscriber.wait(timeout=3) == 0
            no_history = subprocess.run([command("mosquitto_sub"), *address, "-t", TOPIC, "-C", "1", "-W", "1"],
                                        capture_output=True, text=True, timeout=4)
            assert no_history.returncode != 0 and not no_history.stdout
            print("[TEST] INT-08 PASS real loopback Mosquitto: encoded JSON received by terminal subscriber, broker refusal, subscriber reconnect, no retained history")
            print("Fixture:", received)
            print("Evidence boundary: Mac CLI transport only; Pico radio and firmware connection not exercised.")
        finally:
            for process in reversed(processes):
                if process.poll() is None:
                    process.terminate()
                    process.wait(timeout=3)
                if process.stdout:
                    process.stdout.close()


if __name__ == "__main__":
    main()

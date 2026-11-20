# System (SYS) test runbook

## Contents

- [Before you start](#before-you-start)
- [Which image to flash](#which-image-to-flash)
- [Captures](#captures)
- [Run the test code](#run-the-test-code)
- [Every video](#every-video)
- [After each test](#after-each-test)
- [Tests](#tests)
  - [SYS-U01](#sys-u01-full-course-run)

How to run and record SYS-U01. Test definitions: [TEST_LOG.md](TEST_LOG.md).

## Before you start

1. Start from the submission checkout and use the [build profiles](../../README.md#build-profiles) below to identify the required image.
2. Commit nothing else while testing; the image is built from the working tree, so `git status` must be clean. Note the commit: `git rev-parse --short HEAD`.
3. Follow the [power policy](../../README.md#power-and-flashing): USB alone for flashing and TEST images; battery alone for driving.
4. Broker (one terminal, leave it running):

   ```bash
   mosquitto -c config/mosquitto-lab.conf
   ```

5. A GP20 start is refused until letters are mapped. After each boot, map four letters (left, right, straight, U-turn):

   ```bash
   ~/.robot-car-tools/venv/bin/python scripts/mqtt_client.py map A B C D --broker 127.0.0.1
   ```

6. Dashboard for the video (another terminal), then open <http://127.0.0.1:8080> (or <http://localhost:8080>):

   ```bash
   ~/.robot-car-tools/venv/bin/python scripts/mqtt_client.py serve --broker 127.0.0.1
   ```

## Which image to flash

Host test code (`sh testing/tools/run.sh <ID>`) runs on the laptop and needs no flash. Tests on the car need the image named in their **Image** line. Run every test of one image before flashing the next, top to bottom; a test listed under two images uses both (see its steps):

| Image | Power | Tests |
| --- | --- | --- |
| `trial.uf2` | Battery only | SYS-U01 |

Before every flash:

1. Compare the broker host’s Wi-Fi address (`ip -4 -brief address` on Linux) with `MQTT_BROKER_IP` in `config/wifi_secrets.h`. If the hotspot gave the laptop a new address, edit that line first: the address is built into the image, and a wrong one leaves the dashboard empty.
2. `git status` is clean, so the commit you record is the code you flashed.

## Captures

Every test saves its logs in `testing/system/logs/<test ID>/`, named `<what>.log`. Start each capture **before** resetting the Pico; stop it with Ctrl-C when the test ends.

USB console (replace `<ID>`; USB images only, never during a battery run):

```bash
mkdir -p testing/system/logs/<ID> && cat /dev/ttyACM0 | tee testing/system/logs/<ID>/usb.log
```

Telemetry with laptop timestamps (every frame, nothing folded):

```bash
~/.robot-car-tools/venv/bin/python -u scripts/mqtt_client.py monitor --raw --broker 127.0.0.1 | perl -MTime::HiRes=time -ne 'printf "%.3f %s", time, $_' | tee testing/system/logs/<ID>/monitor.log
```

## Run the test code

Host test code for a test ID is in `logs/<ID>/` beside its logs. It runs the real subsystem code with fake hardware, so it needs no robot. Run it before flashing:

```bash
sh testing/tools/run.sh <ID>   # one test ID, for example the first one below
sh testing/tools/run.sh        # every host test
```

A manual test with no code prints `no tests for <ID>`. SYS-U01 has no on-car automatic check; it is run by hand with the steps below.

## Every video

1. Hold up or say the test ID.
2. Show `git rev-parse --short HEAD` in a terminal.
3. Show the hardware and the capture terminal while the test runs.
4. End on the result (console line, dashboard or tape measure).
5. Upload to YouTube as **Unlisted** and add it to the [video index](../../video/README.md).

## After each test

Give the system owner the test ID, the video link and anything you measured (tape readings, counts). The owner checks the logs and makes **one commit per test**: the logs plus one result row in [TEST_LOG.md](TEST_LOG.md), message `test(system): <ID> <PASS|FAIL|PARTIAL>`.

## Tests

### SYS-U01 Full course run

**Image:** `trial.uf2` (full course with diagnostics; battery only)

**Set up:** A line with a junction; a barcode mapped to a turn before the junction; a hump on a straight; a box on the line with both sides open. Fresh boot on the battery, letters mapped, dashboard open, telemetry capture started.

**Steps:**

1. Press GP20.
2. Do not touch the car while it follows the line, crosses the hump, stops at the box, bypasses it, rejoins the line, reads the barcode and turns at the junction.
3. End the run with GP21, or by lifting the car (lift stop).

**Show on video:** The whole run with the dashboard: the hump count and height, the stop at the box, the bypass and rejoin, the barcode letter and the turn, and the stop at the end.

**Test code:** `logs/SYS-U01/test_line_replay.c`, `logs/SYS-U01/test_line_trace.c`, `logs/SYS-U01/test_mission.c`, `logs/SYS-U01/test_mission_pending.c`, `logs/SYS-U01/test_smooth_navigation.c`, `logs/SYS-U01/test_safety.c`, `logs/SYS-U01/test_robot_data.c`, `logs/SYS-U01/test_robot_time.c`, `logs/SYS-U01/test_trace_ring.c`

**Run:** `sh testing/tools/run.sh SYS-U01`, then flash the image above, start the captures and follow the steps.

**Logs:** `monitor.log`; read from it the mapping ACK, `line=`, each HUMP height, `front=` at rest after the scan, the bypass side and clearance, each BARCODE and TURN event, and the telemetry rate.

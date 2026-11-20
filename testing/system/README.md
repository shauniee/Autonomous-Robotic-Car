# System test runbook

## Contents

- [Before you start](#before-you-start)
- [Which image to flash](#which-image-to-flash)
- [Captures](#captures)
- [Run the test code](#run-the-test-code)
- [Every video](#every-video)
- [After the test](#after-the-test)
- [SYS-U01 Full course run](#sys-u01-full-course-run)

How to run and record SYS-U01, the one full course run that tests the whole robot. Test definition: [TEST_LOG.md](TEST_LOG.md); ready-to-run checks: [mission readiness](mission-readiness.md).

## Before you start

1. Work on a fresh branch from `main`: `git fetch` then `git switch -c test/sys-u01 origin/main`.
2. Commit nothing else while testing; the image is built from the working tree, so `git status` must be clean. Note the commit: `git rev-parse --short HEAD`.
3. Follow the [power policy](../../README.md#power-and-flashing): USB alone for flashing; battery alone for driving.
4. Broker (one terminal, leave it running):

   ```bash
   "$(brew --prefix)/sbin/mosquitto" -c config/mosquitto-lab.conf
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

Host test code (`sh testing/tools/run.sh SYS-U01`) runs on the laptop and needs no flash.

| Image | Power | Tests |
| --- | --- | --- |
| `./flash.sh trial` | Battery only | SYS-U01 |

Before every flash:

1. `ipconfig getifaddr en0` must print the `MQTT_BROKER_IP` in `config/wifi_secrets.h`. If the hotspot gave the laptop a new address, edit that line first: the address is built into the image, and a wrong one leaves the dashboard empty.
2. `git status` is clean, so the commit you record is the code you flashed.

## Captures

The run saves its log in `testing/system/logs/SYS-U01/monitor.log`. There is no USB console during a battery run. Start the capture **before** switching the car on; stop it with Ctrl-C when the run ends.

Telemetry with laptop timestamps (every frame, nothing folded):

```bash
~/.robot-car-tools/venv/bin/python -u scripts/mqtt_client.py monitor --raw --broker 127.0.0.1 | perl -MTime::HiRes=time -ne 'printf "%.3f %s", time, $_' | tee testing/system/logs/SYS-U01/monitor.log
```

## Run the test code

Host test code for SYS-U01 is in `logs/SYS-U01/` beside its log. It runs the real Mission, line, bypass, navigation, safety and telemetry code with fake hardware, so it needs no robot. Run it before flashing:

```bash
sh testing/tools/run.sh SYS-U01   # SYS-U01 only
sh testing/tools/run.sh           # every host test, as CI runs it
```

## Every video

1. Hold up or say the test ID.
2. Show `git rev-parse --short HEAD` in a terminal.
3. Show the hardware and the capture terminal while the test runs.
4. End on the result (console line, dashboard or tape measure).
5. Upload to YouTube as **Unlisted** and add it to the [video index](../../video/README.md).

## After the test

Give the system owner the video link and anything you measured. The owner checks the log and makes **one commit**: the log plus one result row in [TEST_LOG.md](TEST_LOG.md), message `test(system): SYS-U01 <PASS|FAIL>`.

## SYS-U01 Full course run

**Image:** `./flash.sh trial` (full course with diagnostics; battery only)

**Course:** a line with a junction; a barcode mapped to a turn before the junction; a hump on a straight; a box on the line with both sides open.

**Steps:**

1. Fresh boot on the battery, broker running, letters mapped, dashboard open. Start the capture and the video.
2. Press GP20. Do not touch the car while it follows the line, crosses the hump, stops at the box, bypasses it, rejoins, reads the barcode and turns.
3. End the run with GP21, or by lifting the car (lift stop).

**Metrics**, all read from `monitor.log` and the video:

| Metric | Evidence | Pass |
| --- | --- | --- |
| Mapping | `REQUEST ACK` before the start | Letters accepted over MQTT, no recompile |
| Line following | `line=` in telemetry | VISIBLE throughout |
| Hump | HUMP event, `humps=` and height | Counted, height reported, line held |
| Obstacle stop | `front=` at rest after SCAN | Stops near 150 mm, no contact |
| Bypass and rejoin | OBSTACLE event, then FOLLOW_LINE | Passes on a free side and rejoins the line |
| Barcode turn | BARCODE then TURN event | Turn taken at the next junction; exit on the new line |
| Telemetry | Events and frame times | Each event once; telemetry throughout |
| Stop | `state=STOPPED` at the end | GP21 or lift stops the car locally |

**Test code:** `logs/SYS-U01/test_line_replay.c`, `logs/SYS-U01/test_line_trace.c`, `logs/SYS-U01/test_mission.c`, `logs/SYS-U01/test_mission_pending.c`, `logs/SYS-U01/test_smooth_navigation.c`, `logs/SYS-U01/test_safety.c`, `logs/SYS-U01/test_robot_data.c`, `logs/SYS-U01/test_robot_time.c`, `logs/SYS-U01/test_trace_ring.c`

**Run:** `sh testing/tools/run.sh SYS-U01`, then flash the image above, start the capture and follow the steps.

**Logs:** `monitor.log`

# System test runbook

## Contents

- [Before you start](#before-you-start)
- [Which image to flash](#which-image-to-flash)
- [Captures](#captures)
- [Run the test code](#run-the-test-code)
- [Every video](#every-video)
- [One full course run](#one-full-course-run)

No system test is kept (see [TEST_LOG.md](TEST_LOG.md)). This page is the runbook for the [one full course run](#one-full-course-run) that records the integration tests; ready-to-run checks: [mission readiness](mission-readiness.md).

## Before you start

1. Work on `main` (or the PR branch under test) and `git pull`.
2. Commit nothing else while testing; the image is built from the working tree, so `git status` must be clean. Note the commit: `git rev-parse --short HEAD`.
3. Follow the [power policy](../../README.md#power-and-flashing): USB alone for flashing and TEST images; battery alone for driving.
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

Host test code (`sh testing/tools/run.sh <ID>`) runs on the laptop and needs no flash. Tests on the car need the image named in their **Image** line. Run every test of one image before flashing the next, top to bottom; a test listed under two images uses both (see its steps):

| Image | Power | Tests |
| --- | --- | --- |
| `./flash.sh trial` | Battery only | INT-03, INT-07, INT-08, INT-09, INT-10 |

Before every flash:

1. `ipconfig getifaddr en0` must print the `MQTT_BROKER_IP` in `config/wifi_secrets.h`. If the hotspot gave the laptop a new address, edit that line first: the address is built into the image, and a wrong one leaves the dashboard empty.
2. `git status` is clean, so the commit you record is the code you flashed.

## Captures

The full course run saves its logs in `testing/system/logs/SYS-04/` (kept for the run's evidence), named `<what>.log`. Start each capture **before** resetting the Pico; stop it with Ctrl-C when the test ends.

USB console (TEST images only; never during a battery run):

```bash
mkdir -p testing/system/logs/SYS-04 && cat /dev/cu.usbmodemMTK3PICO1 | tee testing/system/logs/SYS-04/usb.log
```

Telemetry with laptop timestamps (every frame, nothing folded):

```bash
~/.robot-car-tools/venv/bin/python -u scripts/mqtt_client.py monitor --raw --broker 127.0.0.1 | perl -MTime::HiRes=time -ne 'printf "%.3f %s", time, $_' | tee testing/system/logs/SYS-04/monitor.log
```

## Run the test code

Host test code kept from the dropped system tests runs the real code with fake hardware, so it needs no robot: `logs/SYS-04/test_robot_data.c`, `logs/SYS-04/test_robot_time.c`, `logs/SYS-04/test_trace_ring.c` and `logs/SYS-06/test_safety.c`.

```bash
sh testing/tools/run.sh SYS-04   # or SYS-06
sh testing/tools/run.sh          # every host test, as CI runs it
```

## Every video

1. Hold up or say the test ID.
2. Show `git rev-parse --short HEAD` in a terminal.
3. Show the hardware and the capture terminal while the test runs.
4. End on the result (console line, dashboard or tape measure).
5. Upload to YouTube as **Unlisted** and add it to the [video index](../../video/README.md).

## One full course run

One run on `./flash.sh trial` (battery only) records every kept integration test. Save its capture once, in `logs/SYS-04/monitor.log` (no USB during a battery run), film it as one continuous video, and give every test it covers its own result row in [integration/TEST_LOG.md](../integration/TEST_LOG.md) that points to that file and that video.

**Course:** a line with a junction; a barcode mapped to a turn before the junction; a hump on a straight; a box on the line with both sides open.

**Steps:**

1. Fresh boot on the battery, broker running, letters mapped, dashboard open. Start the capture and the video.
2. Press GP20. Do not touch the car while it follows the line, crosses the hump, stops at the box, bypasses it, rejoins, reads the barcode and turns.
3. End the run with GP21, or by lifting the car (the S4-U04 lift stop).

**Covers:**

| Test | Evidence |
| --- | --- |
| INT-03 Barcode then a turn | BARCODE then TURN event; exit on the new line |
| INT-07 Hump while following | HUMP event and height; line stays VISIBLE |
| INT-08 Obstacle stop | `front=` at rest after the stop |
| INT-09 Bypass and rejoin | OBSTACLE event, then FOLLOW_LINE again |
| INT-10 Telemetry during a run | Each event once on the dashboard; telemetry throughout |

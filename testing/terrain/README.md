# Terrain (S4) test runbook

## Contents

- [Before you start](#before-you-start)
- [Which image to flash](#which-image-to-flash)
- [Captures](#captures)
- [Run the test code](#run-the-test-code)
- [Every video](#every-video)
- [After each test](#after-each-test)
- [Tests](#tests)
  - [S4-U01](#s4-u01-imu-boot-calibration)
  - [S4-U02](#s4-u02-hump-count)
  - [S4-U03](#s4-u03-hump-height)
  - [S4-U04](#s4-u04-mission-stop-on-lift-and-flip)

How to run and record S4-U01 to S4-U04. Test definitions: [TEST_LOG.md](TEST_LOG.md).

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
| `test.uf2` built with `TERRAIN_DIAGNOSTICS=1` | USB only | S4-U01, S4-U02, S4-U03 |
| `trial.uf2` | Battery only | S4-U04 |

Before every flash:

1. Compare the broker host’s Wi-Fi address (`ip -4 -brief address` on Linux) with `MQTT_BROKER_IP` in `config/wifi_secrets.h`. If the hotspot gave the laptop a new address, edit that line first: the address is built into the image, and a wrong one leaves the dashboard empty.
2. `git status` is clean, so the commit you record is the code you flashed.

## Captures

Every test saves its logs in `testing/terrain/logs/<test ID>/`, named `<what>.log`. Start each capture **before** resetting the Pico; stop it with Ctrl-C when the test ends.

USB console (replace `<ID>`):

```bash
mkdir -p testing/terrain/logs/<ID> && cat /dev/ttyACM0 | tee testing/terrain/logs/<ID>/usb.log
```

Telemetry with laptop timestamps (every frame, nothing folded):

```bash
~/.robot-car-tools/venv/bin/python -u scripts/mqtt_client.py monitor --raw --broker 127.0.0.1 | perl -MTime::HiRes=time -ne 'printf "%.3f %s", time, $_' | tee testing/terrain/logs/<ID>/monitor.log
```

## Run the test code

Host test code for a test ID is in `logs/<ID>/` beside its logs. It runs the real subsystem code with fake hardware, so it needs no robot. Run it before flashing:

```bash
sh testing/tools/run.sh <ID>   # one test ID, for example the first one below
sh testing/tools/run.sh        # every host test
```

A manual test with no code prints `no tests for <ID>`. The on-car check for S4-U01 is in `app_program/terrain/terrain_test.c`; `test.uf2` runs it at power-up and prints `[TEST] <ID> PASS|FAIL`.

## Every video

1. Hold up or say the test ID.
2. Show `git rev-parse --short HEAD` in a terminal.
3. Show the hardware and the capture terminal while the test runs.
4. End on the result (console line, dashboard or tape measure).
5. Upload to YouTube as **Unlisted** and add it to the [video index](../../video/README.md).

## After each test

Give the subsystem owner the test ID, the video link and anything you measured (tape readings, counts). The owner checks the logs and makes **one commit per test**: the logs plus one result row in [TEST_LOG.md](TEST_LOG.md), message `test(terrain): <ID> <PASS|FAIL|PARTIAL>`.

## Tests

### S4-U01 IMU boot calibration

**Image:** `test.uf2` built with `TERRAIN_DIAGNOSTICS=1`

**Set up:** USB capture started.

**Steps:**

1. Boot with the car still: `[IMU] calibration passed`.
2. Boot while rocking the car: `[IMU] calibration failed`.
3. Power off, unplug the IMU, boot: calibration fails; press GP20 and the start is refused.

**Show on video:** Each `[IMU]` line, the refused start and, on the still boot, the self-check line `[TEST] S4-U01 PASS`.

**Test code:** `app_program/terrain/terrain_test.c` (on the car)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log`

### S4-U02 Hump count

**Image:** `test.uf2` built with `TERRAIN_DIAGNOSTICS=1`

**Set up:** 10 mm hump on the floor; captures started.

**Steps:**

1. Push the car by hand so one wheel crosses the hump.
2. Pivot the car by hand on the spot.
3. Repeat until three crossings.

**Show on video:** Each crossing and `humps=` on the dashboard.

**Test code:** `logs/S4-U02/test_terrain.c`

**Run:** `sh testing/tools/run.sh S4-U02`, then flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` or `monitor.log`

### S4-U03 Hump height

**Image:** `test.uf2` built with `TERRAIN_DIAGNOSTICS=1`

**Set up:** A 10 mm hump, height measured; captures started.

**Steps:**

1. Push one wheel over the hump three times.

**Show on video:** The measured height and the reported height.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` or `monitor.log`; write down the hump's measured height.

### S4-U04 Mission stop on lift and flip

**Image:** `trial.uf2` (battery). The TEST image cannot be used: it keeps the motors inhibited, so GP21 never clears a stop.

**Set up:** Booted still on the floor; telemetry shows `state=READY`; no run started; telemetry capture started.

**Steps:**

1. Lift the car straight up off the floor, hold it for a second, then set it down flat.
2. Wait a few seconds, then press and release GP21.
3. Turn the car over, then set it back on its wheels.
4. Wait a few seconds, then press and release GP21.

**Show on video:** Each lift and flip, `Terrain: lifted` and `state=STOPPED` on the dashboard, and READY again after GP21.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `monitor.log`

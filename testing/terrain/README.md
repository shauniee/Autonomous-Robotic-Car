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
  - [S4-U02](#s4-u02-tilt-and-lift)
  - [S4-U03](#s4-u03-hump-count)
  - [S4-U04](#s4-u04-hump-height)
  - [S4-U05](#s4-u05-hump-under-the-other-wheel-and-in-reverse)

How to run and record S4-U01 to S4-U05. Test definitions: [TEST_LOG.md](TEST_LOG.md).

## Before you start

1. Work on this branch: `git switch test/s4-terrain` and `git pull`.
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
| `TERRAIN_DIAGNOSTICS=1 ./flash.sh test` | USB only | S4-U01, S4-U03, S4-U04, S4-U05 |
| `./flash.sh trial` | Battery only | S4-U02 |

Before every flash:

1. `ipconfig getifaddr en0` must print the `MQTT_BROKER_IP` in `config/wifi_secrets.h`. If the hotspot gave the laptop a new address, edit that line first: the address is built into the image, and a wrong one leaves the dashboard empty.
2. `git status` is clean, so the commit you record is the code you flashed.

## Captures

Every test saves its logs in `testing/terrain/logs/<test ID>/`, named `<what>.log`. Start each capture **before** resetting the Pico; stop it with Ctrl-C when the test ends.

USB console (replace `<ID>`):

```bash
mkdir -p testing/terrain/logs/<ID> && cat /dev/cu.usbmodemMTK3PICO1 | tee testing/terrain/logs/<ID>/usb.log
```

Telemetry with laptop timestamps (every frame, nothing folded):

```bash
~/.robot-car-tools/venv/bin/python -u scripts/mqtt_client.py monitor --raw --broker 127.0.0.1 | perl -MTime::HiRes=time -ne 'printf "%.3f %s", time, $_' | tee testing/terrain/logs/<ID>/monitor.log
```

## Run the test code

Host test code for a test ID is in `logs/<ID>/` beside its logs. It runs the real subsystem code with fake hardware, so it needs no robot. Run it before flashing:

```bash
sh testing/tools/run.sh <ID>   # one test ID, for example the first one below
sh testing/tools/run.sh        # every host test, as CI runs it
```

A manual test with no code prints `no tests for <ID>`. The on-car check for S4-U01 is in `app_program/terrain/terrain_test.c`; `./flash.sh test` runs it at power-up and prints `[TEST] <ID> PASS|FAIL`.

## Every video

1. Hold up or say the test ID.
2. Show `git rev-parse --short HEAD` in a terminal.
3. Show the hardware and the capture terminal while the test runs.
4. End on the result (console line, dashboard or tape measure).
5. Upload to YouTube as **Unlisted** and add it to the [video index](../../video/README.md).

## After each test

Send Claude the test ID, the video link and anything you measured (tape readings, counts). Claude checks the logs and makes **one commit per test**: the logs plus one result row in [TEST_LOG.md](TEST_LOG.md), message `test(terrain): <ID> <PASS|FAIL|PARTIAL>`.

## Tests

### S4-U01 IMU boot calibration

**Image:** `TERRAIN_DIAGNOSTICS=1 ./flash.sh test`

**Set up:** USB capture started.

**Steps:**

1. Boot with the car still: `[IMU] calibration passed`.
2. Boot while rocking the car: `[IMU] calibration failed`.
3. Power off, unplug the IMU, boot: calibration fails; press GP20 and the start is refused.

**Show on video:** Each `[IMU]` line, the refused start and, on the still boot, the self-check line `[TEST] S4-U01 PASS`.

**Test code:** `app_program/terrain/terrain_test.c` (on the car)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log`

### S4-U02 Tilt and lift

**Image:** `./flash.sh trial` (battery; it sends the IMU trace)

**Set up:** Booted still on the floor; letters mapped; telemetry capture started; dashboard IMU graphs open.

**Steps:**

1. Lift the nose about 20 degrees and hold, then put it down.
2. Press GP20 to start a run. Lift the car straight up off the floor.
3. Check the wheels stop and the dashboard shows `Terrain: lifted` and a latched stop. Put the car down; press and release GP21 to clear it.
4. Start again and turn the car over.
5. Repeat steps 2 to 4 three times.

**Show on video:** Pitch rising on the dashboard; each lift and turn over, the wheels stopping and `Terrain: lifted`.

**Test code:** `logs/S4-U02/test_capture_imu.py`

**Run:** `sh testing/tools/run.sh S4-U02`, then flash the image above, start the captures and follow the steps.

**Logs:** `monitor.log`. Claude reads the vertical axis (`z=`) in the trace at each lift and on the floor. Set `LIFT_ACCEL_G` (now 0.3 g, provisional D06) from that trace.

### S4-U03 Hump count

**Image:** `TERRAIN_DIAGNOSTICS=1 ./flash.sh test`

**Set up:** 10 mm hump on the floor; captures started.

**Steps:**

1. Push the car by hand so one wheel crosses the hump.
2. Pivot the car by hand on the spot.
3. Repeat until three crossings.

**Show on video:** Each crossing and `humps=` on the dashboard.

**Test code:** `logs/S4-U03/test_terrain.c`

**Run:** `sh testing/tools/run.sh S4-U03`, then flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`

### S4-U04 Hump height

**Image:** `TERRAIN_DIAGNOSTICS=1 ./flash.sh test`

**Set up:** Humps of measured heights from 10 to 30 mm; captures started.

**Steps:**

1. Push one wheel over each hump three times.

**Show on video:** The measured height and the reported height.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`; write down each hump's measured height.

### S4-U05 Hump under the other wheel and in reverse

**Image:** `TERRAIN_DIAGNOSTICS=1 ./flash.sh test`

**Set up:** 10 mm hump; captures started.

**Steps:**

1. Left wheel forward, left wheel reverse.
2. Right wheel forward, right wheel reverse.

**Show on video:** Each crossing and the side reported.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`

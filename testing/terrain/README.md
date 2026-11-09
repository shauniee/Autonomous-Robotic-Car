# Terrain (S4) test runbook

## Contents

- [Before you start](#before-you-start)
- [Captures](#captures)
- [Every video](#every-video)
- [After each test](#after-each-test)
- [Tests](#tests)
  - [S4-U01](#s4-u01-imu-boot-calibration)
  - [S4-U02](#s4-u02-tilt-and-impact)
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

6. Dashboard for the video (another terminal), then open <http://127.0.0.1:8080>:

   ```bash
   ~/.robot-car-tools/venv/bin/python scripts/mqtt_client.py serve --broker 127.0.0.1
   ```

## Captures

Every test saves its logs in `testing/terrain/logs/<test ID>/`, named `YYYY-MM-DD_<what>.log`. Start each capture **before** resetting the Pico; stop it with Ctrl-C when the test ends.

USB console (replace `<ID>` and the date):

```bash
mkdir -p testing/terrain/logs/<ID> && cat /dev/cu.usbmodemMTK3PICO1 | tee testing/terrain/logs/<ID>/YYYY-MM-DD_usb.log
```

Telemetry with laptop timestamps (every frame, nothing folded):

```bash
~/.robot-car-tools/venv/bin/python -u scripts/mqtt_client.py monitor --raw --broker 127.0.0.1 | perl -MTime::HiRes=time -ne 'printf "%.3f %s", time, $_' | tee testing/terrain/logs/<ID>/YYYY-MM-DD_monitor.log
```

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

**Show on video:** Each `[IMU]` line and the refused start.

**Logs:** `usb.log`

### S4-U02 Tilt and impact

**Image:** `TERRAIN_DIAGNOSTICS=1 ./flash.sh test`

**Set up:** Booted still; captures started; dashboard IMU graphs open.

**Steps:**

1. Lift the nose about 20 degrees and hold.
2. Put it down; tap the chassis sharply.

**Show on video:** Pitch rising on the dashboard; `Terrain: impact` and the latched stop.

**Logs:** `usb.log` and `monitor.log`

### S4-U03 Hump count

**Image:** `TERRAIN_DIAGNOSTICS=1 ./flash.sh test`

**Set up:** 10 mm hump on the floor; captures started.

**Steps:**

1. Push the car by hand so one wheel crosses the hump.
2. Pivot the car by hand on the spot.
3. Repeat until three crossings.

**Show on video:** Each crossing and `humps=` on the dashboard.

**Logs:** `usb.log` and `monitor.log`

### S4-U04 Hump height

**Image:** `TERRAIN_DIAGNOSTICS=1 ./flash.sh test`

**Set up:** Humps of measured heights from 10 to 30 mm; captures started.

**Steps:**

1. Push one wheel over each hump three times.

**Show on video:** The measured height and the reported height.

**Logs:** `usb.log` and `monitor.log`; write down each hump's measured height.

### S4-U05 Hump under the other wheel and in reverse

**Image:** `TERRAIN_DIAGNOSTICS=1 ./flash.sh test`

**Set up:** 10 mm hump; captures started.

**Steps:**

1. Left wheel forward, left wheel reverse.
2. Right wheel forward, right wheel reverse.

**Show on video:** Each crossing and the side reported.

**Logs:** `usb.log` and `monitor.log`

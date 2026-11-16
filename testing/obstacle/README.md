# Obstacle (S5) test runbook

## Contents

- [Before you start](#before-you-start)
- [Captures](#captures)
- [Run the test code](#run-the-test-code)
- [Every video](#every-video)
- [After each test](#after-each-test)
- [Tests](#tests)
  - [S5-U01](#s5-u01-sonar-range)
  - [S5-U02](#s5-u02-servo-centre-and-limits)
  - [S5-U03](#s5-u03-scan-profile-width)
  - [S5-U04](#s5-u04-bypass-side-from-the-scan)
  - [S5-U05](#s5-u05-no-echo-and-out-of-range)

How to run and record S5-U01 to S5-U05. Wheels never move: all five use the TEST image. Test definitions: [TEST_LOG.md](TEST_LOG.md).

## Before you start

1. Work on this branch: `git switch test/s5-obstacle` and `git pull`.
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

## Captures

Every test saves its logs in `testing/obstacle/logs/<test ID>/`, named `YYYY-MM-DD_<what>.log`. Start each capture **before** resetting the Pico; stop it with Ctrl-C when the test ends.

USB console (replace `<ID>` and the date):

```bash
mkdir -p testing/obstacle/logs/<ID> && cat /dev/cu.usbmodemMTK3PICO1 | tee testing/obstacle/logs/<ID>/YYYY-MM-DD_usb.log
```

Telemetry with laptop timestamps (every frame, nothing folded):

```bash
~/.robot-car-tools/venv/bin/python -u scripts/mqtt_client.py monitor --raw --broker 127.0.0.1 | perl -MTime::HiRes=time -ne 'printf "%.3f %s", time, $_' | tee testing/obstacle/logs/<ID>/YYYY-MM-DD_monitor.log
```

## Run the test code

Host test code for a test ID is in `logs/<ID>/` beside its logs. It runs the real subsystem code with fake hardware, so it needs no robot. Run it before flashing:

```bash
sh testing/tools/run.sh <ID>   # one test ID, for example the first one below
sh testing/tools/run.sh        # every host test, as CI runs it
```

A manual test with no code prints `no tests for <ID>`. The on-car check for S5-U01 is in `app_program/obstacle/obstacle_test.c`; `./flash.sh test` runs it at power-up and prints `[TEST] <ID> PASS|FAIL`.

## Every video

1. Hold up or say the test ID.
2. Show `git rev-parse --short HEAD` in a terminal.
3. Show the hardware and the capture terminal while the test runs.
4. End on the result (console line, dashboard or tape measure).
5. Upload to YouTube as **Unlisted** and add it to the [video index](../../video/README.md).

## After each test

Send Claude the test ID, the video link and anything you measured (tape readings, counts). Claude checks the logs and makes **one commit per test**: the logs plus one result row in [TEST_LOG.md](TEST_LOG.md), message `test(obstacle): <ID> <PASS|FAIL|PARTIAL>`.

## Tests

### S5-U01 Sonar range

**Image:** `./flash.sh test` (wireless, motors always off, USB)

**Set up:** Tape measure from the sonar face; flat board; captures started.

**Steps:**

1. Board at 100 mm; hold 5 s.
2. 250, 500 and 1000 mm.
3. Aim at open space.

**Show on video:** The tape and the dashboard `front=` reading at each distance. At boot, with the board in range, the image also prints its self-check `[TEST] S5-U01 PASS` (front echo valid); show it.

**Test code:** `logs/S5-U01/test_echo_range.c`, `app_program/obstacle/obstacle_test.c` (on the car)

**Run:** `sh testing/tools/run.sh S5-U01`, then flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`; write down the tape distances.

### S5-U02 Servo centre and limits

**Image:** `./flash.sh test` (wireless, motors always off, USB)

**Set up:** USB capture started.

**Steps:**

1. Put a box ahead and press GP20 to start a scan.
2. Watch the servo reach centre and both sweep limits.

**Show on video:** The servo at centre and at each limit, with no stall.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` (`[SCAN]` rays).

### S5-U03 Scan profile width

**Image:** `./flash.sh test` (wireless, motors always off, USB)

**Set up:** Boxes 100, 150 and 200 mm wide, 300 mm ahead on the line; captures started.

**Steps:**

1. Press GP20 to scan each box.
2. Measure each box's edges against the tape.

**Show on video:** Each box, the tape and the `[SCAN]` width line.

**Test code:** `logs/S5-U03/test_obstacle_profile.c`, `logs/S5-U03/test_profile_edges.c`

**Run:** `sh testing/tools/run.sh S5-U03`, then flash the image above, start the captures and follow the steps.

**Logs:** `usb.log`; write down the real widths.

### S5-U04 Bypass side from the scan

**Image:** `./flash.sh test` (wireless, motors always off, USB)

**Set up:** One box 300 mm ahead; captures started.

**Steps:**

1. Box centred: scan three times.
2. Box offset left: three scans.
3. Box offset right: three scans.
4. Box centred, with a wall or a second box less than 220 mm past one edge: three scans. Then move it to the other edge: three scans.

The side is chosen like this:

- If exactly one side shows at least `MISSION_BYPASS_MIN_CLEAR_MM` (220 mm) of clearance, the car takes that side (step 4).
- Otherwise it takes the side with the shorter edge; a tie goes left (steps 1 to 3). Clearance 0 means no echo, not blocked.

**Show on video:** The box position and the side chosen each time.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`

### S5-U05 No echo and out of range

**Image:** `./flash.sh test` (wireless, motors always off, USB)

**Set up:** Captures started.

**Steps:**

1. Aim at open space.
2. Aim at a soft object (cloth).
3. Aim at a board angled 45 degrees.

**Show on video:** No obstacle reported and no stop.

**Test code:** `logs/S5-U05/test_range_filter.c`

**Run:** `sh testing/tools/run.sh S5-U05`, then flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`

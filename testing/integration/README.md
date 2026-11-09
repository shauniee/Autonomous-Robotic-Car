# Integration test runbook

## Contents

- [Before you start](#before-you-start)
- [Captures](#captures)
- [Every video](#every-video)
- [After each test](#after-each-test)
- [Tests](#tests)
  - [INT-01](#int-01-line-following-three-laps)
  - [INT-02](#int-02-line-loss-and-recovery)
  - [INT-03](#int-03-barcode-then-left-and-right-turns)
  - [INT-04](#int-04-barcode-then-straight-and-u-turn)
  - [INT-05](#int-05-junction-or-curve-with-no-turn-queued)
  - [INT-06](#int-06-turn-with-one-wheel-on-a-hump)
  - [INT-07](#int-07-hump-while-line-following)
  - [INT-08](#int-08-obstacle-stop-on-the-line)
  - [INT-09](#int-09-bypass-and-rejoin)
  - [INT-10](#int-10-telemetry-and-events-during-a-run)

How to run and record INT-01 to INT-10. These drive on the course on battery. Test definitions: [TEST_LOG.md](TEST_LOG.md).

## Before you start

1. Work on this branch: `git switch test/integration` and `git pull`.
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

Every test saves its logs in `testing/integration/logs/<test ID>/`, named `YYYY-MM-DD_<what>.log`. Start each capture **before** resetting the Pico; stop it with Ctrl-C when the test ends.

USB console (replace `<ID>` and the date):

```bash
mkdir -p testing/integration/logs/<ID> && cat /dev/cu.usbmodemMTK3PICO1 | tee testing/integration/logs/<ID>/YYYY-MM-DD_usb.log
```

Telemetry with laptop timestamps (every frame, nothing folded):

```bash
~/.robot-car-tools/venv/bin/python -u scripts/mqtt_client.py monitor --raw --broker 127.0.0.1 | perl -MTime::HiRes=time -ne 'printf "%.3f %s", time, $_' | tee testing/integration/logs/<ID>/YYYY-MM-DD_monitor.log
```

## Every video

1. Hold up or say the test ID.
2. Show `git rev-parse --short HEAD` in a terminal.
3. Show the hardware and the capture terminal while the test runs.
4. End on the result (console line, dashboard or tape measure).
5. Upload to YouTube as **Unlisted** and add it to the [video index](../../video/README.md).

## After each test

Send Claude the test ID, the video link and anything you measured (tape readings, counts). Claude checks the logs and makes **one commit per test**: the logs plus one result row in [TEST_LOG.md](TEST_LOG.md), message `test(integration): <ID> <PASS|FAIL|PARTIAL>`.

## Tests

### INT-01 Line following, three laps

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Loop with straights and curves; letters mapped; captures started.

**Steps:**

1. Press GP20.
2. Let it run three laps; press GP21.

**Show on video:** The whole run, filmed from above if possible.

**Logs:** `usb.log` and `monitor.log`

### INT-02 Line loss and recovery

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Straight line with a removable section; captures started.

**Steps:**

1. Start following.
2. Lift the line section away in front of the car.

**Show on video:** The sweep finding the line, or the stop.

**Logs:** `usb.log` and `monitor.log`

### INT-03 Barcode then left and right turns

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Letter mapped to LEFT before a junction; captures started.

**Steps:**

1. Run over the letter and through the junction.
2. Swap to a letter mapped to RIGHT and repeat.

**Show on video:** The letter card, then the turn at the junction.

**Logs:** `usb.log` and `monitor.log`

### INT-04 Barcode then straight and U-turn

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Letters mapped to STRAIGHT and U-turn.

**Steps:**

1. Run each letter through the junction.

**Show on video:** The car crossing straight, then the U-turn.

**Logs:** `usb.log` and `monitor.log`

### INT-05 Junction or curve with no turn queued

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** No letter before the junction; tightest curve on the course.

**Steps:**

1. Run through the junction and the curve.

**Show on video:** The car continuing without stopping.

**Logs:** `usb.log` and `monitor.log`

### INT-06 Turn with one wheel on a hump

**Image:** bench image (battery)

**Set up:** Hump under one wheel at the taped square.

**Steps:**

1. Repeat the S2-U03 pivots, five of each.

**Show on video:** Each pivot against the tape.

**Logs:** `usb.log`; write down the errors. Needs the S2 bench image.

### INT-07 Hump while line following

**Image:** `TERRAIN_DIAGNOSTICS=1 ./flash.sh trial`

**Set up:** Hump on a straight, then on a curve.

**Steps:**

1. Follow over each.

**Show on video:** The car holding the line and `humps=` rising.

**Logs:** `usb.log` and `monitor.log`; write down the hump height.

### INT-08 Obstacle stop on the line

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Board on the line ahead.

**Steps:**

1. Start following toward it.

**Show on video:** The car slowing and stopping; tape the gap.

**Logs:** `usb.log` and `monitor.log`; write down the gap.

### INT-09 Bypass and rejoin

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Box on the line.

**Steps:**

1. Both sides open: run.
2. One side blocked: run.
3. Both sides blocked: run.

**Show on video:** Each bypass and the rejoin, or the stop when blocked.

**Logs:** `usb.log` and `monitor.log`

### INT-10 Telemetry and events during a run

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Course with a barcode, junction, hump and box; dashboard on screen.

**Steps:**

1. Run the course.
2. Mid-run, stop the broker for 10 s and restart it.

**Show on video:** Each event on the dashboard, and the car carrying on during the loss.

**Logs:** `usb.log` and `monitor.log`

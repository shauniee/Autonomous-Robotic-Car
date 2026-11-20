# Integration test runbook

## Contents

- [Before you start](#before-you-start)
- [Which image to flash](#which-image-to-flash)
- [Captures](#captures)
- [Run the test code](#run-the-test-code)
- [Every video](#every-video)
- [After each test](#after-each-test)
- [One full course run](#one-full-course-run)
- [Tests](#tests)
  - [INT-03](#int-03-barcode-then-a-turn)
  - [INT-07](#int-07-hump-while-line-following)
  - [INT-08](#int-08-obstacle-stop-on-the-line)
  - [INT-09](#int-09-bypass-and-rejoin)
  - [INT-10](#int-10-telemetry-and-events-during-a-run)

How to run and record INT-03 and INT-07 to INT-10, all from [one full course run](#one-full-course-run). These drive on the course on battery. Test definitions: [TEST_LOG.md](TEST_LOG.md).

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

Every test saves its logs in `testing/integration/logs/<test ID>/`, named `<what>.log`. Start each capture **before** resetting the Pico; stop it with Ctrl-C when the test ends.

USB console (replace `<ID>`):

```bash
mkdir -p testing/integration/logs/<ID> && cat /dev/cu.usbmodemMTK3PICO1 | tee testing/integration/logs/<ID>/usb.log
```

Telemetry with laptop timestamps (every frame, nothing folded):

```bash
~/.robot-car-tools/venv/bin/python -u scripts/mqtt_client.py monitor --raw --broker 127.0.0.1 | perl -MTime::HiRes=time -ne 'printf "%.3f %s", time, $_' | tee testing/integration/logs/<ID>/monitor.log
```

## Run the test code

Host test code for a test ID is in `logs/<ID>/` beside its logs. It runs the real subsystem code with fake hardware, so it needs no robot. Run it before flashing:

```bash
sh testing/tools/run.sh <ID>   # one test ID, for example the first one below
sh testing/tools/run.sh        # every host test, as CI runs it
```

A manual test with no code prints `no tests for <ID>`. No test here has an on-car automatic check; each one is run by hand with the steps below.

## Every video

1. Hold up or say the test ID.
2. Show `git rev-parse --short HEAD` in a terminal.
3. Show the hardware and the capture terminal while the test runs.
4. End on the result (console line, dashboard or tape measure).
5. Upload to YouTube as **Unlisted** and add it to the [video index](../../video/README.md).

## After each test

Send Claude the test ID, the video link and anything you measured (tape readings, counts). Claude checks the logs and makes **one commit per test**: the logs plus one result row in [TEST_LOG.md](TEST_LOG.md), message `test(integration): <ID> <PASS|FAIL|PARTIAL>`.

## One full course run

Every integration test is recorded from the single scripted course run in the [system runbook](../system/README.md#one-full-course-run). Its captures are saved once, in `testing/system/logs/SYS-04/`; each INT result row here points to them and to that run's video. INT-02 and INT-06 were dropped, and INT-01, INT-04 and INT-05 on 2026-11-20 (not needed for the demo); their IDs are not reused.

## Tests

### INT-03 Barcode then a turn

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Letter mapped to LEFT or RIGHT before a junction; captures started.

**Steps:**

1. Run over the letter and through the junction.

**Show on video:** The letter card, then the turn at the junction.

**Test code:** `logs/INT-03/test_line_replay.c`, `logs/INT-03/test_line_trace.c`

**Run:** `sh testing/tools/run.sh INT-03`, then flash the image above, start the captures and follow the steps.

**Logs:** `monitor.log`

### INT-07 Hump while line following

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Hump on a straight, and on a curve if the course has one.

**Steps:**

1. Follow over each.

**Show on video:** The car holding the line and `humps=` rising.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`; write down the hump height.

### INT-08 Obstacle stop on the line

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Board on the line ahead.

**Steps:**

1. Start following toward it.

**Show on video:** The car slowing and stopping.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `monitor.log`; the gap is the `front=` reading at rest.

### INT-09 Bypass and rejoin

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Box on the line.

**Steps:**

1. Both sides open: run. The car takes the side with the shorter edge, passes the box and rejoins the line.

**Show on video:** The bypass and the rejoin.

**Test code:** `logs/INT-09/test_mission.c`, `logs/INT-09/test_mission_pending.c`, `logs/INT-09/test_smooth_navigation.c`

**Run:** `sh testing/tools/run.sh INT-09`, then flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`

### INT-10 Telemetry and events during a run

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Course with a barcode, junction, hump and box; dashboard on screen.

**Steps:**

1. Run the course.

**Show on video:** Each event on the dashboard and the turn rate (Turn under Motor speed, from the encoders, FR-20) changing in the turns.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`

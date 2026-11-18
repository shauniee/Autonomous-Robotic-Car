# System test runbook

## Contents

- [Before you start](#before-you-start)
- [Which image to flash](#which-image-to-flash)
- [Captures](#captures)
- [Run the test code](#run-the-test-code)
- [Every video](#every-video)
- [After each test](#after-each-test)
- [One full course run](#one-full-course-run)
- [Tests](#tests)
  - [SYS-03](#sys-03-mission-day-routine)
  - [SYS-04](#sys-04-full-course)
  - [SYS-06](#sys-06-stop-and-reset)
  - [SYS-08](#sys-08-network-never-drives)
  - [SYS-09](#sys-09-endless-operation)

How to run and record SYS-03, SYS-04, SYS-06, SYS-08 and SYS-09, all from [one full course run](#one-full-course-run). Test definitions: [TEST_LOG.md](TEST_LOG.md); ready-to-run checks: [mission readiness](mission-readiness.md).

## Before you start

1. Work on this branch: `git switch test/system` and `git pull`.
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
| `./flash.sh trial` | Battery only | SYS-03, SYS-04, SYS-06, SYS-08, SYS-09 |

Before every flash:

1. `ipconfig getifaddr en0` must print the `MQTT_BROKER_IP` in `config/wifi_secrets.h`. If the hotspot gave the laptop a new address, edit that line first: the address is built into the image, and a wrong one leaves the dashboard empty.
2. `git status` is clean, so the commit you record is the code you flashed.

## Captures

Every test saves its logs in `testing/system/logs/<test ID>/`, named `YYYY-MM-DD_<what>.log`. Start each capture **before** resetting the Pico; stop it with Ctrl-C when the test ends.

USB console (replace `<ID>` and the date):

```bash
mkdir -p testing/system/logs/<ID> && cat /dev/cu.usbmodemMTK3PICO1 | tee testing/system/logs/<ID>/YYYY-MM-DD_usb.log
```

Telemetry with laptop timestamps (every frame, nothing folded):

```bash
~/.robot-car-tools/venv/bin/python -u scripts/mqtt_client.py monitor --raw --broker 127.0.0.1 | perl -MTime::HiRes=time -ne 'printf "%.3f %s", time, $_' | tee testing/system/logs/<ID>/YYYY-MM-DD_monitor.log
```

## Run the test code

Host test code for a test ID is in `logs/<ID>/` beside its logs. It runs the real subsystem code with fake hardware, so it needs no robot. Run it before flashing:

```bash
sh testing/tools/run.sh <ID>   # one test ID, for example the first one below
sh testing/tools/run.sh        # every host test, as CI runs it
```

A manual test with no code prints `no tests for <ID>`.

## Every video

1. Hold up or say the test ID.
2. Show `git rev-parse --short HEAD` in a terminal.
3. Show the hardware and the capture terminal while the test runs.
4. End on the result (console line, dashboard or tape measure).
5. Upload to YouTube as **Unlisted** and add it to the [video index](../../video/README.md).

## After each test

Send Claude the test ID, the video link and anything you measured (tape readings, counts). Claude checks the logs and makes **one commit per test**: the logs plus one result row in [TEST_LOG.md](TEST_LOG.md), message `test(system): <ID> <PASS|FAIL|PARTIAL>`.

## One full course run

One scripted run on `./flash.sh trial` (battery only) records every system and integration test. Save its captures once, in `logs/SYS-04/` (`monitor.log`; no USB during a battery run), film it as one continuous video, and give every test it covers its own result row that points to those files and that video.

**Course:** line with straights and the tightest curve; four barcodes (left, right, straight, U-turn), each before a junction; one junction with no barcode before it; a hump on a straight, and one on a curve if possible; a box on the line with both sides open.

**Steps:**

1. Fresh boot on the battery, broker running, map four letters not used before, dashboard open. Start the captures and the video.
2. Press GP20 and start a stopwatch. Do not touch the car for three laps.
3. During lap 2, stop the broker for about 10 s, then start it again.
4. After lap 3, at course speed, press GP21. Tape the stopping distance.
5. Hold GP20: nothing starts. Release it.
6. While stopped, send `map` and `snapshot` requests from the laptop: the car does not move.
7. Reset with a fresh GP21 press and release, then press GP20 fresh: it starts. Stop it with GP21.

**Covers** every system and integration test that remains:

| Test | Evidence |
| --- | --- |
| SYS-03 Mission-day routine | Step 1, local start |
| SYS-04 Full course | Lap time from the stopwatch |
| SYS-06 Stop and reset | Steps 4, 5 and 7 |
| SYS-08 Network never drives | Steps 3 and 6 |
| SYS-09 Endless operation | Three laps over the same junctions, GP21 ends it |
| INT-01 Line following, three laps | Three laps |
| INT-03 Left and right turns | Left and right barcodes |
| INT-04 Straight and U-turn | Straight and U-turn barcodes |
| INT-05 Junction or curve, no turn queued | Unqueued junction and the tightest curve |
| INT-07 Hump while following | The hump on the straight, and on the curve if the course has one |
| INT-08 Obstacle stop | Rest distance from `front=` at the scan, or taped |
| INT-09 Bypass and rejoin | The box with both sides open |
| INT-10 Telemetry during a run | Each event once on the dashboard; step 3 |

SYS-01, SYS-02, SYS-05, SYS-07, SYS-10, INT-02 and INT-06 were dropped so that this one run is the whole system and integration record. Their IDs are not reused.

## Tests

### SYS-03 Mission-day routine

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Fresh boot on battery; letters you have not used before.

**Steps:**

1. Boot still.
2. Start broker and dashboard.
3. Map the letters.
4. Press GP20.

**Show on video:** Each step, with no laptop code change.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `monitor.log`

### SYS-04 Full course

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Course with line, barcodes, junctions, a hump and an obstacle.

**Steps:**

1. Press GP20 and start a stopwatch.
2. Let it finish the course; stop the watch.

**Show on video:** The whole run; say the time.

**Test code:** `logs/SYS-04/test_robot_data.c`, `logs/SYS-04/test_robot_time.c`, `logs/SYS-04/test_trace_ring.c`

**Run:** `sh testing/tools/run.sh SYS-04`, then flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`; write down the run time.

### SYS-06 Stop and reset

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Straight line, tape at the stop point.

**Steps:**

1. Press GP21 at course speed.
2. Hold GP20: nothing starts.
3. Release, press GP20 fresh: it starts.

**Show on video:** The stop, the tape and the fresh start.

**Test code:** `logs/SYS-06/test_safety.c`

**Run:** `sh testing/tools/run.sh SYS-06`, then flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`; write down the stopping distance.

### SYS-08 Network never drives

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Driving on the line.

**Steps:**

1. Stop the broker mid-run.
2. Start it again.
3. While stopped, send `map` and `snapshot` requests.

**Show on video:** The car keeping its own state; no start from the laptop.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`

### SYS-09 Endless operation

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Loop with one junction.

**Steps:**

1. Run several loops, with and without a letter before the junction.
2. Press GP21 to end.

**Show on video:** Each loop behaving the same.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`

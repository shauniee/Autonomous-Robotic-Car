# System test runbook

## Contents

- [Before you start](#before-you-start)
- [Captures](#captures)
- [Run the test code](#run-the-test-code)
- [Every video](#every-video)
- [After each test](#after-each-test)
- [Tests](#tests)
  - [SYS-01](#sys-01-electrical-readiness)
  - [SYS-02](#sys-02-test-image-self-checks-auto)
  - [SYS-03](#sys-03-mission-day-routine)
  - [SYS-04](#sys-04-full-course)
  - [SYS-05](#sys-05-three-runs-in-a-row)
  - [SYS-06](#sys-06-stop-and-reset)
  - [SYS-07](#sys-07-fault-handling)
  - [SYS-08](#sys-08-network-never-drives)
  - [SYS-09](#sys-09-endless-operation)
  - [SYS-10](#sys-10-timing-and-memory-under-full-load)

How to run and record SYS-01 to SYS-10. Test definitions: [TEST_LOG.md](TEST_LOG.md); ready-to-run checks: [mission readiness](mission-readiness.md).

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

A manual test with no code prints `no tests for <ID>`. SYS-02 runs every subsystem's on-car checks with `./flash.sh test`.

## Every video

1. Hold up or say the test ID.
2. Show `git rev-parse --short HEAD` in a terminal.
3. Show the hardware and the capture terminal while the test runs.
4. End on the result (console line, dashboard or tape measure).
5. Upload to YouTube as **Unlisted** and add it to the [video index](../../video/README.md).

## After each test

Send Claude the test ID, the video link and anything you measured (tape readings, counts). Claude checks the logs and makes **one commit per test**: the logs plus one result row in [TEST_LOG.md](TEST_LOG.md), message `test(system): <ID> <PASS|FAIL|PARTIAL>`.

## Tests

### SYS-01 Electrical readiness

**Image:** none (power off for continuity)

**Set up:** Multimeter; [pin table](../../docs/team.md#2-pins).

**Steps:**

1. Continuity of every wire (MR-01).
2. Jumpers and contacts (MR-02).
3. Sonar echo level (MR-03).
4. Power polarity (MR-04).
5. 3V3 and VSYS with motors and servo running (MR-05).

**Show on video:** The meter on each reading.

**Test code:** none (manual test)

**Run:** no image; follow the steps with the board unpowered except for step 5.

**Logs:** a typed `readings.log` of each value.

### SYS-02 TEST image self-checks (auto)

**Image:** `./flash.sh test` (wireless, motors always off, USB)

**Set up:** USB capture started; wheels free; car still; a board about 250 mm in front of the sonar.

**Steps:**

1. Reset the Pico.
2. Wait for `[TEST] === test run end ===`.
3. Press GP21, then GP20: nothing moves.

The image checks each subsystem and prints one line each:

- `S1-U05` frame codec
- `S2-U05` command freshness gate
- `S3-U01` line IR sample valid
- `S4-U01` IMU calibration passed (boot still)
- `S5-U01` front sonar echo valid (needs the board in range)

**Show on video:** Every `[TEST]` line PASS and the wheels still.

**Test code:** every `app_program/<subsystem>/<subsystem>_test.c` (on the car: S1-U05, S2-U05, S3-U01, S4-U01, S5-U01)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log`

### SYS-03 Mission-day routine

**Image:** `./flash.sh` (course image)

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

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`; write down the run time.

### SYS-05 Three runs in a row

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Same as SYS-04, no code or constant change.

**Steps:**

1. Run SYS-04 two more times.

**Show on video:** Each run.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log` for each run.

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

### SYS-07 Fault handling

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Driving on the line.

**Steps:**

1. Wiggle an encoder lead.
2. Reset; start again and lift the car off the floor (`Terrain: lifted`).
3. Power off, unplug the IMU, boot.

**Show on video:** Each stop and the fault name on the dashboard.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`

### SYS-08 Network never drives

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** Driving on the line.

**Steps:**

1. Turn WiFi off at the hotspot mid-run.
2. Turn it back on.
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

### SYS-10 Timing and memory under full load

**Image:** `./flash.sh trial` (full course with diagnostics; battery for driving)

**Set up:** USB console captured during a full course (can be the SYS-04 run).

**Steps:**

1. Run the full course.

**Show on video:** The `[TIMING]` lines on the USB terminal.

**Test code:** `logs/SYS-10/test_robot_data.c`, `logs/SYS-10/test_robot_time.c`, `logs/SYS-10/test_trace_ring.c`

**Run:** `sh testing/tools/run.sh SYS-10`, then flash the image above, start the captures and follow the steps.

**Logs:** `usb.log`; Claude reads `[TIMING]` max, overruns and stack.

# Motion (S2) test runbook

## Contents

- [Before you start](#before-you-start)
- [Captures](#captures)
- [Run the test code](#run-the-test-code)
- [Every video](#every-video)
- [After each test](#after-each-test)
- [Bench image](#bench-image)
- [Tests](#tests)
  - [S2-U01](#s2-u01-motor-direction-and-encoder-counts)
  - [S2-U02](#s2-u02-1-m-straight)
  - [S2-U03](#s2-u03-90-and-180-degree-pivots)
  - [S2-U04](#s2-u04-wheel-speed-control-on-the-floor)
  - [S2-U05](#s2-u05-command-freshness-gate-auto)

How to run and record S2-U01 to S2-U05. Test definitions: [TEST_LOG.md](TEST_LOG.md).

## Before you start

1. Work on this branch: `git switch test/s2-motion` and `git pull`.
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

Every test saves its logs in `testing/motion/logs/<test ID>/`, named `<what>.log`. Start each capture **before** resetting the Pico; stop it with Ctrl-C when the test ends.

USB console (replace `<ID>`):

```bash
mkdir -p testing/motion/logs/<ID> && cat /dev/cu.usbmodemMTK3PICO1 | tee testing/motion/logs/<ID>/usb.log
```

Telemetry with laptop timestamps (every frame, nothing folded):

```bash
~/.robot-car-tools/venv/bin/python -u scripts/mqtt_client.py monitor --raw --broker 127.0.0.1 | perl -MTime::HiRes=time -ne 'printf "%.3f %s", time, $_' | tee testing/motion/logs/<ID>/monitor.log
```

## Run the test code

Host test code for a test ID is in `logs/<ID>/` beside its logs. It runs the real subsystem code with fake hardware, so it needs no robot. Run it before flashing:

```bash
sh testing/tools/run.sh <ID>   # one test ID, for example the first one below
sh testing/tools/run.sh        # every host test, as CI runs it
```

A manual test with no code prints `no tests for <ID>`. The on-car check for S2-U05 is in `app_program/motion/motion_test.c`; `./flash.sh test` runs it at power-up and prints `[TEST] <ID> PASS|FAIL`.

## Every video

1. Hold up or say the test ID.
2. Show `git rev-parse --short HEAD` in a terminal.
3. Show the hardware and the capture terminal while the test runs.
4. End on the result (console line, dashboard or tape measure).
5. Upload to YouTube as **Unlisted** and add it to the [video index](../../video/README.md).

## After each test

Send Claude the test ID, the video link and anything you measured (tape readings, counts). Claude checks the logs and makes **one commit per test**: the logs plus one result row in [TEST_LOG.md](TEST_LOG.md), message `test(motion): <ID> <PASS|FAIL|PARTIAL>`.

## Bench image

`./flash.sh bench` never runs the mission. Each GP20 press runs the next single move, at course speed, in this order:

1. `short_forward` (100 mm)
2. `straight_1m` (1000 mm)
3. `left_90`
4. `right_90`
5. `turn_180`

Then it starts again at 1. Each move prints one `[BENCH]` line with the move, the result (`DONE`, `FAILED` or `ABORTED`), the left and right counts, the travel in mm, the heading and the time. A move that does not finish is run again on the next press. GP21 still stops; if a press prints `[BENCH] refused`, press and release GP21 first. The last `[BENCH]` line tells you which move is next.

## Tests

### S2-U01 Motor direction and encoder counts

**Image:** `./flash.sh bench` for the motor check; `./flash.sh test` (wireless, motors always off, USB) for the hand turns

**Set up:** Mark a spot on each tyre. Wheels off the ground for the motor check.

**Steps:**

1. Bench image: press GP20 once for `short_forward`; both wheels must turn forward and the `[BENCH]` line shows both counts rising.
2. TEST image: turn the left wheel exactly one revolution by hand, three times, reading `counts L` on the dashboard before and after each.
3. Repeat for the right wheel.

**Show on video:** Both wheels turning forward; the count before and after each revolution.

**Test code:** `logs/S2-U01/test_encoder_math.c`

**Run:** `sh testing/tools/run.sh S2-U01`, then flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`; write down the counts.

### S2-U02 1 m straight

**Image:** `./flash.sh bench` (battery)

**Set up:** Tape a start line and a 1 m mark; mark the car's centre.

**Steps:**

1. Place the car on the start line.
2. Press GP20 for `straight_1m`.
3. Tape the stop point: distance and sideways error.
4. Repeat five times (step through the other four moves between runs).

**Show on video:** Each run and the tape measurement.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` (`[BENCH]` lines); write down the five distances and errors.

### S2-U03 90 and 180 degree pivots

**Image:** `./flash.sh bench` (battery)

**Set up:** Tape a square on the floor; align the car with one edge.

**Steps:**

1. Press GP20 for each pivot in turn: `left_90`, `right_90`, `turn_180`.
2. Measure the heading error against the tape.
3. Five of each.

**Show on video:** Each pivot ending against the tape.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` (`[BENCH]` lines); write down the 15 heading errors.

### S2-U04 Wheel speed control on the floor

**Image:** `./flash.sh bench` (battery; it sends `WHEEL` traces)

**Set up:** Telemetry capture started.

**Steps:**

1. Press GP20 for `straight_1m` (course speed, 100 mm/s).
2. Let it finish.

**Show on video:** The dashboard wheel-speed graph following the target.

**Test code:** `logs/S2-U04/test_motion_trace.c`

**Run:** `sh testing/tools/run.sh S2-U04`, then flash the image above, start the captures and follow the steps.

**Logs:** `monitor.log` (`WHEEL` traces).

### S2-U05 Command freshness gate (auto)

**Image:** `./flash.sh test` (wireless, motors always off, USB)

**Set up:** USB capture started; wheels free.

**Steps:**

1. Reset the Pico.
2. Wait for `[TEST] === test run end ===`.

**Show on video:** The line `[TEST] S2-U05 PASS` and the wheels not moving.

**Test code:** `logs/S2-U05/test_motion_control.c`, `app_program/motion/motion_test.c` (on the car)

**Run:** `sh testing/tools/run.sh S2-U05`, then flash the image above, start the captures and follow the steps.

**Logs:** `usb.log`

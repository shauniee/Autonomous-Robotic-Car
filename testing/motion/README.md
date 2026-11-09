# Motion (S2) test runbook

## Contents

- [Before you start](#before-you-start)
- [Captures](#captures)
- [Every video](#every-video)
- [After each test](#after-each-test)
- [Needed before recording](#needed-before-recording)
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

6. Dashboard for the video (another terminal), then open <http://127.0.0.1:8080>:

   ```bash
   ~/.robot-car-tools/venv/bin/python scripts/mqtt_client.py serve --broker 127.0.0.1
   ```

## Captures

Every test saves its logs in `testing/motion/logs/<test ID>/`, named `YYYY-MM-DD_<what>.log`. Start each capture **before** resetting the Pico; stop it with Ctrl-C when the test ends.

USB console (replace `<ID>` and the date):

```bash
mkdir -p testing/motion/logs/<ID> && cat /dev/cu.usbmodemMTK3PICO1 | tee testing/motion/logs/<ID>/YYYY-MM-DD_usb.log
```

Telemetry with laptop timestamps (every frame, nothing folded):

```bash
~/.robot-car-tools/venv/bin/python -u scripts/mqtt_client.py monitor --raw --broker 127.0.0.1 | perl -MTime::HiRes=time -ne 'printf "%.3f %s", time, $_' | tee testing/motion/logs/<ID>/YYYY-MM-DD_monitor.log
```

## Every video

1. Hold up or say the test ID.
2. Show `git rev-parse --short HEAD` in a terminal.
3. Show the hardware and the capture terminal while the test runs.
4. End on the result (console line, dashboard or tape measure).
5. Upload to YouTube as **Unlisted** and add it to the [video index](../../video/README.md).

## After each test

Send Claude the test ID, the video link and anything you measured (tape readings, counts). Claude checks the logs and makes **one commit per test**: the logs plus one result row in [TEST_LOG.md](TEST_LOG.md), message `test(motion): <ID> <PASS|FAIL|PARTIAL>`.

## Needed before recording

- **S2-U01 to S2-U04 need a bench image.** No current image drives a single 1 m straight or a pivot on its own. A `./flash.sh bench` image, where each GP20 press runs the next move (short forward, 1 m straight, 90 left, 90 right, 180) and prints a `[BENCH]` line, must be added on this branch first; ask Claude to build it.

## Tests

### S2-U01 Motor direction and encoder counts

**Image:** bench image for the motor check; `./flash.sh test` (wireless, motors always off, USB) for the hand turns

**Set up:** Mark a spot on each tyre. Wheels off the ground for the motor check.

**Steps:**

1. Bench image: press GP20 once for the short forward move; both wheels must turn forward.
2. TEST image: turn the left wheel exactly one revolution by hand, three times, reading `counts L` on the dashboard before and after each.
3. Repeat for the right wheel.

**Show on video:** Both wheels turning forward; the count before and after each revolution.

**Logs:** `usb.log` and `monitor.log`; write down the counts.

### S2-U02 1 m straight

**Image:** bench image (battery)

**Set up:** Tape a start line and a 1 m mark; mark the car's centre.

**Steps:**

1. Place the car on the start line.
2. Press GP20 for the 1 m move.
3. Tape the stop point: distance and sideways error.
4. Repeat five times.

**Show on video:** Each run and the tape measurement.

**Logs:** `usb.log` (`[BENCH]` lines); write down the five distances and errors.

### S2-U03 90 and 180 degree pivots

**Image:** bench image (battery)

**Set up:** Tape a square on the floor; align the car with one edge.

**Steps:**

1. Press GP20 for each pivot in turn: 90 left, 90 right, 180.
2. Measure the heading error against the tape.
3. Five of each.

**Show on video:** Each pivot ending against the tape.

**Logs:** `usb.log`; write down the 15 heading errors.

### S2-U04 Wheel speed control on the floor

**Image:** bench image (battery)

**Set up:** Telemetry capture started.

**Steps:**

1. Press GP20 for the 1 m move at 100 mm/s.
2. Let it finish.

**Show on video:** The dashboard wheel-speed graph following the target.

**Logs:** `monitor.log` (`WHEEL` traces).

### S2-U05 Command freshness gate (auto)

**Image:** `./flash.sh test` (wireless, motors always off, USB)

**Set up:** USB capture started; wheels free.

**Steps:**

1. Reset the Pico.
2. Wait for `[TEST] === test run end ===`.

**Show on video:** The line `[TEST] S2-U05 PASS` and the wheels not moving.

**Logs:** `usb.log`

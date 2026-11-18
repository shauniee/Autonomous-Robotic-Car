# Line and barcode (S3) test runbook

## Contents

- [Before you start](#before-you-start)
- [Captures](#captures)
- [Run the test code](#run-the-test-code)
- [Every video](#every-video)
- [After each test](#after-each-test)
- [Tests](#tests)
  - [S3-U01](#s3-u01-line-ir-levels)
  - [S3-U02](#s3-u02-barcode-ir-level)
  - [S3-U03](#s3-u03-all-26-letters)
  - [S3-U04](#s3-u04-reverse-direction-and-flipped-print)
  - [S3-U05](#s3-u05-read-rate-at-course-speed)

How to run and record S3-U01 to S3-U05. Test definitions: [TEST_LOG.md](TEST_LOG.md).

## Before you start

1. Work on this branch: `git switch test/s3-line-barcode` and `git pull`.
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

Every test saves its logs in `testing/line_barcode/logs/<test ID>/`, named `<what>.log`. Start each capture **before** resetting the Pico; stop it with Ctrl-C when the test ends.

USB console (replace `<ID>`):

```bash
mkdir -p testing/line_barcode/logs/<ID> && cat /dev/cu.usbmodemMTK3PICO1 | tee testing/line_barcode/logs/<ID>/usb.log
```

Telemetry with laptop timestamps (every frame, nothing folded):

```bash
~/.robot-car-tools/venv/bin/python -u scripts/mqtt_client.py monitor --raw --broker 127.0.0.1 | perl -MTime::HiRes=time -ne 'printf "%.3f %s", time, $_' | tee testing/line_barcode/logs/<ID>/monitor.log
```

## Run the test code

Host test code for a test ID is in `logs/<ID>/` beside its logs. It runs the real subsystem code with fake hardware, so it needs no robot. Run it before flashing:

```bash
sh testing/tools/run.sh <ID>   # one test ID, for example the first one below
sh testing/tools/run.sh        # every host test, as CI runs it
```

A manual test with no code prints `no tests for <ID>`. The on-car check for S3-U01 is in `app_program/line_barcode/line_barcode_test.c`; `./flash.sh test` runs it at power-up and prints `[TEST] <ID> PASS|FAIL`.

## Every video

1. Hold up or say the test ID.
2. Show `git rev-parse --short HEAD` in a terminal.
3. Show the hardware and the capture terminal while the test runs.
4. End on the result (console line, dashboard or tape measure).
5. Upload to YouTube as **Unlisted** and add it to the [video index](../../video/README.md).

## After each test

Send Claude the test ID, the video link and anything you measured (tape readings, counts). Claude checks the logs and makes **one commit per test**: the logs plus one result row in [TEST_LOG.md](TEST_LOG.md), message `test(line_barcode): <ID> <PASS|FAIL|PARTIAL>`.

## Tests

### S3-U01 Line IR levels

**Image:** `./flash.sh test` (wireless, motors always off, USB)

**Set up:** Car on the bench over white; USB capture started.

**Steps:**

1. Slide black under the left sensor (GP26), then off.
2. Same for the right sensor (GP17).
3. Hold each over the line edge.

**Show on video:** The `[IR] … black=L/R … line=…` lines changing with each move. At boot the image also prints its self-check, `[TEST] S3-U01 PASS` (line IR sample valid); show it.

**Test code:** `logs/S3-U01/test_ir_tracking.c`, `app_program/line_barcode/line_barcode_test.c` (on the car)

**Run:** `sh testing/tools/run.sh S3-U01`, then flash the image above, start the captures and follow the steps.

**Logs:** `usb.log`

### S3-U02 Barcode IR level

**Image:** `BARCODE_DIAGNOSTICS=1 ./flash.sh test`

**Set up:** Car on the bench; USB capture started.

**Steps:**

1. Slide a black strip slowly under the barcode sensor (GP27), then white.
2. Repeat three times.
3. Read the module's potentiometer position.

**Show on video:** `[BCPIN] level=` flipping and `edges=` counting up; say the pot setting.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log`; note the pot setting.

### S3-U03 All 26 letters

**Image:** `BARCODE_DIAGNOSTICS=1 ./flash.sh trial` (battery)

**Set up:** Straight line with one printed letter at a time; letters mapped; captures started.

**Steps:**

1. Press GP20; let the car drive over the letter.
2. Stop with GP21, change the letter, repeat for A to Z.

**Show on video:** Each letter on the floor and the dashboard letter card.

**Test code:** `logs/S3-U03/test_barcode.c`

**Run:** `sh testing/tools/run.sh S3-U03`, then flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`

### S3-U04 Reverse direction and flipped print

**Image:** `BARCODE_DIAGNOSTICS=1 ./flash.sh trial` (battery)

**Set up:** Same as S3-U03 with five letters.

**Steps:**

1. Drive each letter in the reverse direction.
2. Turn the print 180 degrees and drive again.

**Show on video:** The letter card matching each print.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`

### S3-U05 Read rate at course speed

**Image:** `BARCODE_DIAGNOSTICS=1 ./flash.sh trial` (battery)

**Set up:** One letter on a straight line; captures started.

**Steps:**

1. Drive over it ten times.

**Show on video:** Each pass and the letter card.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`; Claude counts reads against passes.

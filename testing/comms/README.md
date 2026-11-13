# Comms (S1) test runbook

## Contents

- [Before you start](#before-you-start)
- [Captures](#captures)
- [Every video](#every-video)
- [After each test](#after-each-test)
- [Tests](#tests)
  - [S1-U01](#s1-u01-wifi-join-and-telemetry-rate)
  - [S1-U02](#s1-u02-letter-mapping-rules)
  - [S1-U03](#s1-u03-broker-restart-while-stopped)
  - [S1-U04](#s1-u04-malformed-and-prohibited-requests)
  - [S1-U05](#s1-u05-frame-codec-on-the-pico-auto)

How to run and record S1-U01 to S1-U05. Test definitions: [TEST_LOG.md](TEST_LOG.md).

## Before you start

1. Work on this branch: `git switch test/s1-comms` and `git pull`.
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

Every test saves its logs in `testing/comms/logs/<test ID>/`, named `YYYY-MM-DD_<what>.log`. Start each capture **before** resetting the Pico; stop it with Ctrl-C when the test ends.

USB console (replace `<ID>` and the date):

```bash
mkdir -p testing/comms/logs/<ID> && cat /dev/cu.usbmodemMTK3PICO1 | tee testing/comms/logs/<ID>/YYYY-MM-DD_usb.log
```

Telemetry with laptop timestamps (every frame, nothing folded):

```bash
~/.robot-car-tools/venv/bin/python -u scripts/mqtt_client.py monitor --raw --broker 127.0.0.1 | perl -MTime::HiRes=time -ne 'printf "%.3f %s", time, $_' | tee testing/comms/logs/<ID>/YYYY-MM-DD_monitor.log
```

## Every video

1. Hold up or say the test ID.
2. Show `git rev-parse --short HEAD` in a terminal.
3. Show the hardware and the capture terminal while the test runs.
4. End on the result (console line, dashboard or tape measure).
5. Upload to YouTube as **Unlisted** and add it to the [video index](../../video/README.md).

## After each test

Send Claude the test ID, the video link and anything you measured (tape readings, counts). Claude checks the logs and makes **one commit per test**: the logs plus one result row in [TEST_LOG.md](TEST_LOG.md), message `test(comms): <ID> <PASS|FAIL|PARTIAL>`.

## Tests

### S1-U01 WiFi join and telemetry rate

**Image:** `./flash.sh test` (wireless, motors always off, USB)

**Set up:** Broker and dashboard running; USB and telemetry captures started.

**Steps:**

1. Reset the Pico.
2. Wait for `wifi: link up, ip=…` on the USB console.
3. Leave it for 60 s with the dashboard updating.

**Show on video:** The `wifi: link up` line, then the dashboard updating, including Turn under Motor speed (turn rate from the encoders, FR-20; `monitor` prints it as `turn=`).

**Logs:** `usb.log` and `monitor.log`; Claude measures the telemetry interval (target about 500 ms).

### S1-U02 Letter mapping rules

**Image:** `./flash.sh test` (wireless, motors always off, USB)

**Set up:** Booted and connected, with no mapping sent yet (skip step 5); telemetry capture started.

**Steps:**

1. Press GP20: the start is refused (no mapping).
2. Send a mapping: `~/.robot-car-tools/venv/bin/python scripts/mqtt_client.py map A B C D --broker 127.0.0.1`.
3. Check the dashboard shows the mapping.
4. Reset the Pico; check the mapping is gone.

**Show on video:** The refused start, the `map` command and its reply, the dashboard before and after the reset.

**Logs:** `usb.log` and `monitor.log`

### S1-U03 Broker restart while stopped

**Image:** `./flash.sh test` (wireless, motors always off, USB)

**Set up:** Booted, connected, dashboard open; telemetry capture started.

**Steps:**

1. Stop the broker (Ctrl-C in its terminal) and start a stopwatch.
2. After 10 s start it again.
3. Stop the stopwatch when the dashboard updates again.

**Show on video:** The dashboard showing the loss, then telemetry returning; say the recovery time.

**Logs:** `usb.log` and `monitor.log`; note the recovery time.

### S1-U04 Malformed and prohibited requests

**Image:** `./flash.sh test` (wireless, motors always off, USB)

**Set up:** Booted and connected; USB and telemetry (`monitor`) captures started.

**Steps:**

1. In another terminal run the probe. It sends a truncated mapping, a mapping with the old version and a drive request, one line each:

   ```bash
   ~/.robot-car-tools/venv/bin/python scripts/mqtt_client.py probe --broker 127.0.0.1 | tee testing/comms/logs/S1-U04/YYYY-MM-DD_probe.log
   ```

2. Watch `monitor`: no ACK and no state change. Watch the robot: nothing moves.

**Show on video:** The three probe lines, `monitor` with no ACK and no state change, and the robot still idle.

**Logs:** `usb.log`, `monitor.log` and `probe.log`

### S1-U05 Frame codec on the Pico (auto)

**Image:** `./flash.sh test` (wireless, motors always off, USB)

**Set up:** USB capture started.

**Steps:**

1. Reset the Pico.
2. Wait for `[TEST] === test run end ===`.

**Show on video:** The line `[TEST] S1-U05 PASS`.

**Logs:** `usb.log`

# Comms (S1) test runbook

## Contents

- [Before you start](#before-you-start)
- [Which image to flash](#which-image-to-flash)
- [Captures](#captures)
- [Run the test code](#run-the-test-code)
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

1. Start from the submission checkout and use the [build profiles](../../README.md#build-profiles) below to identify the required image.
2. Commit nothing else while testing; the image is built from the working tree, so `git status` must be clean. Note the commit: `git rev-parse --short HEAD`.
3. Follow the [power policy](../../README.md#power-and-flashing): USB alone for flashing and TEST images; battery alone for driving.
4. Broker (one terminal, leave it running):

   ```bash
   mosquitto -c config/mosquitto-lab.conf
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
| `test.uf2` | USB only | S1-U01, S1-U02, S1-U03, S1-U04, S1-U05 |

Before every flash:

1. Compare the broker host’s Wi-Fi address (`ip -4 -brief address` on Linux) with `MQTT_BROKER_IP` in `config/wifi_secrets.h`. If the hotspot gave the laptop a new address, edit that line first: the address is built into the image, and a wrong one leaves the dashboard empty.
2. `git status` is clean, so the commit you record is the code you flashed.

## Captures

Every test saves its logs in `testing/comms/logs/<test ID>/`, named `<what>.log`. Start each capture **before** resetting the Pico; stop it with Ctrl-C when the test ends.

USB console (replace `<ID>`):

```bash
mkdir -p testing/comms/logs/<ID> && cat /dev/ttyACM0 | tee testing/comms/logs/<ID>/usb.log
```

Telemetry with laptop timestamps (every frame, nothing folded):

```bash
~/.robot-car-tools/venv/bin/python -u scripts/mqtt_client.py monitor --raw --broker 127.0.0.1 | perl -MTime::HiRes=time -ne 'printf "%.3f %s", time, $_' | tee testing/comms/logs/<ID>/monitor.log
```

## Run the test code

Host test code for a test ID is in `logs/<ID>/` beside its logs. It runs the real subsystem code with fake hardware, so it needs no robot. Run it before flashing:

```bash
sh testing/tools/run.sh <ID>   # one test ID, for example the first one below
sh testing/tools/run.sh        # every host test
```

A manual test with no code prints `no tests for <ID>`. The on-car check for S1-U05 is in `app_program/comms/comms_test.c`; `test.uf2` runs it at power-up and prints `[TEST] <ID> PASS|FAIL`.

## Every video

1. Hold up or say the test ID.
2. Show `git rev-parse --short HEAD` in a terminal.
3. Show the hardware and the capture terminal while the test runs.
4. End on the result (console line, dashboard or tape measure).
5. Upload to YouTube as **Unlisted** and add it to the [video index](../../video/README.md).

## After each test

Give the subsystem owner the test ID, the video link and anything you measured (tape readings, counts). The owner checks the logs and makes **one commit per test**: the logs plus one result row in [TEST_LOG.md](TEST_LOG.md), message `test(comms): <ID> <PASS|FAIL|PARTIAL>`.

## Tests

### S1-U01 WiFi join and telemetry rate

**Image:** `test.uf2` (wireless, motors always off, USB)

**Set up:** Broker and dashboard running; USB and telemetry captures started.

**Steps:**

1. Reset the Pico.
2. Wait for `wifi: link up, ip=…` on the USB console.
3. Leave it for 60 s with the dashboard updating.

**Show on video:** The `wifi: link up` line, then the dashboard updating, including Turn under Motor speed (turn rate from the encoders, FR-20; `monitor` prints it as `turn=`).

**Test code:** `logs/S1-U01/test_comms_report.c`, `logs/S1-U01/test_comms_stress.c`, `logs/S1-U01/test_dashboard.mjs`, `logs/S1-U01/test_dashboard_http.py`

**Run:** `sh testing/tools/run.sh S1-U01`, then flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`; measure the telemetry interval (target about 500 ms; up to 550 ms accepted).

### S1-U02 Letter mapping rules

**Image:** `test.uf2` (wireless, motors always off, USB)

**Set up:** Booted and connected, with no mapping sent yet (skip step 5); telemetry capture started.

**Steps:**

1. Press GP20: the start is refused (no mapping).
2. Send a mapping: `~/.robot-car-tools/venv/bin/python scripts/mqtt_client.py map A B C D --broker 127.0.0.1`.
3. Check the dashboard shows the mapping.
4. Reset the Pico; check the mapping is gone.

**Show on video:** The refused start, the `map` command and its reply, the dashboard before and after the reset.

**Test code:** `logs/S1-U02/test_mission_mapping.c`

**Run:** `sh testing/tools/run.sh S1-U02`, then flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`

### S1-U03 Broker restart while stopped

**Image:** `test.uf2` (wireless, motors always off, USB)

**Set up:** Booted, connected, dashboard open; telemetry capture started.

**Steps:**

1. Stop the broker (Ctrl-C in its terminal) and start a stopwatch.
2. After 10 s start it again.
3. Stop the stopwatch when the dashboard updates again.

**Show on video:** The dashboard showing the loss, then telemetry returning; say the recovery time.

**Test code:** none (manual test)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log` and `monitor.log`; note the recovery time.

### S1-U04 Malformed and prohibited requests

**Image:** `test.uf2` (wireless, motors always off, USB)

**Set up:** Booted and connected; USB and telemetry (`monitor`) captures started.

**Steps:**

1. In another terminal run the probe. It sends a truncated mapping, a mapping with the old version and a drive request, one line each:

   ```bash
   ~/.robot-car-tools/venv/bin/python scripts/mqtt_client.py probe --broker 127.0.0.1 | tee testing/comms/logs/S1-U04/probe.log
   ```

2. Watch `monitor`: no ACK and no state change. Watch the robot: nothing moves.

**Show on video:** The three probe lines, `monitor` with no ACK and no state change, and the robot still idle.

**Test code:** `logs/S1-U04/test_mqtt_cli.py`

**Run:** `sh testing/tools/run.sh S1-U04`, then flash the image above, start the captures and follow the steps.

**Logs:** `usb.log`, `monitor.log` and `probe.log`

### S1-U05 Frame codec on the Pico (auto)

**Image:** `test.uf2` (wireless, motors always off, USB)

**Set up:** USB capture started.

**Steps:**

1. Reset the Pico.
2. Wait for `[TEST] === test run end ===`.

**Show on video:** The line `[TEST] S1-U05 PASS`.

**Test code:** `logs/S1-U05/test_mqtt_frame.c`, `app_program/comms/comms_test.c` (on the car)

**Run:** `sh testing/tools/run.sh S1-U05`, then flash the image above, start the captures and follow the steps.

**Logs:** `usb.log`

# Comms (S1) — test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)

**Subsystem:** Communications: WiFi, MQTT, telemetry  ·  **Student ID:** 2500629  ·  **Code:** `app_program/comms/`

See [testing/README.md](../README.md) for how to run tests and record results.

## Test cases

Define a test here before running it. Test IDs never change.

| ID | What is tested | Requirement / interface | Method | Steps / how | Expected result |
|---|---|---|---|---|---|
| S1-U01 | WiFi join and telemetry rate | M10, MR-08 | manual | 1. Boot the wireless image on battery. 2. Start the broker and `mqtt_client.py serve`. 3. Watch 1 minute | Pico joins WiFi; telemetry about every 500 ms |
| S1-U02 | Letter mapping rules | MR-09, MR-10 | manual | 1. Press GP20 with no mapping. 2. `map A B C D` while stopped. 3. Reboot | Start refused without a mapping; mapping accepted while stopped; cleared on reboot |
| S1-U03 | Broker restart while stopped | MR-33, T13 | manual | 1. Stop the broker for 10 s. 2. Restart it | Dashboard shows the loss, then telemetry resumes; recovery time recorded |
| S1-U04 | Malformed and prohibited requests | T12 | manual | 1. Send a truncated frame, a wrong-version frame and a request to drive | Each rejected; robot state unchanged |
| S1-U05 | Frame codec on the Pico | M10 | auto | `comms_test.c` on the TEST image | `[TEST] S1-U05 PASS` |

*Method:* **auto** = `TEST_CHECK` in code, result printed on the console;
**manual** = a person observes the result (motor, sensor, dashboard, tape measure).
Each test is shown in one [video](../../video/README.md).

## Results

Add a new row for every run. Never edit or delete old rows.

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
|---|---|---|---|---|---|---|
| YYYY-MM-DD | S1-U01 | `abc1234` | | PASS / FAIL | `logs/<ID>/YYYY-MM-DD_….log`, video link | |
| 2026-11-18 | S1-U01 | `12b3983` | Nicholas | PARTIAL | `logs/S1-U01/2026-11-18_usb.log`, `logs/S1-U01/2026-11-18_monitor.log`; video recorded, not uploaded yet | Joined WiFi (`ip=172.20.10.3`), `mqtt: ready`, stayed READY. Telemetry sent about every 540 ms (frames #9 to #120 in 59.9 s), above the 500 ms target: the 10 ms kernel tick (`CNF_TIMER_PERIOD`) makes the 10 ms I/O task run every 20 ms. 12 of 112 frames lost on the phone hotspot, longest gap 4.1 s. Turn rate shown while the car was rotated by hand (-22 to 2 deg/s). |
| 2026-11-18 | S1-U02 | `12b3983` | Nicholas | PASS | `logs/S1-U02/2026-11-18_usb.log`, `logs/S1-U02/2026-11-18_monitor.log`; video recorded, not uploaded yet | GP20 with no mapping: `start refused: mapped=0`. `map A B C D` while stopped: `REQUEST ACK seq 254: OK`, telemetry `mapped=yes`. After a USB replug: `mapped=NO`. An earlier attempt was void: tilting the car during S1-U01 had latched a lift stop. |
| 2026-11-18 | S1-U03 | `12b3983` | Nicholas | PASS | `logs/S1-U03/2026-11-18_usb.log`, `logs/S1-U03/2026-11-18_monitor.log`, `logs/S1-U03/2026-11-18_events.log`; video recorded, not uploaded yet | Broker stopped for 10 s; telemetry resumed 5.7 s after the restart (16.3 s from the last frame before the stop). The Pico reconnected on its own. |
| 2026-11-18 | S1-U04 | `12b3983` | Nicholas | PASS | `logs/S1-U04/2026-11-18_usb.log`, `logs/S1-U04/2026-11-18_monitor.log`, `logs/S1-U04/2026-11-18_probe.log`; video recorded, not uploaded yet | `mqtt_client.py probe` sent a truncated mapping, a v2 mapping and a drive request. No ACK; all 17 telemetry frames after the probe stayed READY, IDLE, unmapped; wheel duty 0 in every sample. |
| 2026-11-18 | S1-U05 | `12b3983` | Nicholas | PASS | `logs/S1-U05/2026-11-18_usb.log`; video recorded, not uploaded yet | `[TEST] S1-U05 PASS  binary telemetry round-trip and length check`, `comms SUMMARY 1 passed, 0 failed` at power-up on the TEST image. |

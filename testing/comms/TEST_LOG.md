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
| S1-U01 | WiFi join and telemetry rate | M10 | manual | 1. Boot the wireless image on battery. 2. Start the broker and `mqtt_client.py serve`. 3. Watch 1 minute | Pico joins WiFi; telemetry about every 500 ms (up to 550 ms accepted: the 10 ms kernel tick) |
| S1-U02 | Letter mapping rules | M10, NFR-06 | manual | 1. Press GP20 with no mapping. 2. `map A B C D` while stopped. 3. Reboot | Start refused without a mapping; mapping accepted while stopped; cleared on reboot |
| S1-U03 | Broker restart while stopped | T13 | manual | 1. Stop the broker for 10 s. 2. Restart it | Dashboard shows the loss, then telemetry resumes; recovery time recorded |
| S1-U04 | Malformed and prohibited requests | T12 | manual | 1. Send a truncated frame, a wrong-version frame and a request to drive | Each rejected; robot state unchanged |
| S1-U05 | Frame codec on the Pico | M10 | auto | `comms_test.c` on the TEST image | `[TEST] S1-U05 PASS` |

*Method:* **auto** = `TEST_CHECK` in code, result printed on the console;
**manual** = a person observes the result (motor, sensor, dashboard, tape measure).
Each test is shown in one [video](../../video/README.md).

## Results

Add a new row for every run. Never edit or delete old rows.

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
|---|---|---|---|---|---|---|
| 2026-11-18 | S1-U01 | `12b3983` | Shaun | PASS | `logs/S1-U01/usb.log`, `logs/S1-U01/monitor.log`; [Terminal video](https://youtu.be/ZYH8X5pl338) | Joined WiFi and stayed READY. Telemetry averaged about 540 ms per frame (#9 to #120 in 59.9 s), within the 550 ms limit accepted on 2026-11-20. The hotspot lost 12 of 112 frames, with a longest gap of 4.1 s. |
| 2026-11-18 | S1-U02 | `12b3983` | Shaun | PASS | `logs/S1-U02/usb.log`, `logs/S1-U02/monitor.log`; [Terminal video](https://youtu.be/yrx6Bqv97ds) | GP20 with no mapping: `start refused: mapped=0`. `map A B C D` while stopped: `REQUEST ACK seq 254: OK`, telemetry `mapped=yes`. After a USB replug: `mapped=NO`. An earlier attempt was void: tilting the car during S1-U01 had latched a lift stop. |
| 2026-11-18 | S1-U03 | `12b3983` | Shaun | PASS | `logs/S1-U03/usb.log`, `logs/S1-U03/monitor.log`, `logs/S1-U03/events.log`; [Terminal video](https://youtu.be/Afe4-LWxF1s) | Broker stopped for 10 s; telemetry resumed 5.7 s after the restart (16.3 s from the last frame before the stop). The Pico reconnected on its own. |
| 2026-11-18 | S1-U04 | `12b3983` | Shaun | PASS | `logs/S1-U04/usb.log`, `logs/S1-U04/monitor.log`, `logs/S1-U04/probe.log`; [Terminal video](https://youtu.be/zoLosirWVpc) | `mqtt_client.py probe` sent a truncated mapping, a v2 mapping and a drive request. No ACK; all 17 telemetry frames after the probe stayed READY, IDLE, unmapped; wheel duty 0 in every sample. |
| 2026-11-18 | S1-U05 | `12b3983` | Shaun | PASS | `logs/S1-U05/usb.log`; [Terminal video](https://youtu.be/m3s9QtUCYl0) | `[TEST] S1-U05 PASS  binary telemetry round-trip and length check`, `comms SUMMARY 1 passed, 0 failed` at power-up on the TEST image. |

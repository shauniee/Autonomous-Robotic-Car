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

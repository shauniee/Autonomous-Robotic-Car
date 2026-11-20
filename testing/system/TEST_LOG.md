# System — test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)

The test of the complete robot on the course, on the final hardware set-up: one full course run, SYS-U01.

See [testing/README.md](../README.md) for how to run tests and record results.

## Test cases

Define a test here before running it. Test IDs never change.

| ID | What is tested | Requirement / interface | Method | Steps / how | Expected result |
|---|---|---|---|---|---|
| SYS-U01 | Full course run | M1, M3, M4, M5, M6, M8, M9, M10, NFR-06, NFR-07 | manual | 1. Fresh boot on battery; letters mapped over MQTT; dashboard open. 2. Press GP20 and leave the car: line, hump, box on the line, barcode before a junction. 3. End with GP21 or by lifting the car | Line held; hump counted and its height reported; car stops at the box (about 150 mm) and passes it on a free side, then rejoins the line; turn taken at the junction after the barcode; each event once on the dashboard with telemetry throughout; car stops locally at the end |

*Method:* **auto** = `TEST_CHECK` in code, result printed on the console;
**manual** = a person observes the result (motor, sensor, dashboard, tape measure).
Each test is shown in one [video](../../video/README.md).

## Results

Add a new row for every run. Never edit or delete old rows.

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
|---|---|---|---|---|---|---|
| 2026-11-20 | SYS-U01 | `880ff16` (`trial`) | Mahesha | PASS | `logs/SYS-U01/monitor.log`; [Test video](https://youtu.be/PXza6Wc6LAE) | Battery only, fresh boot 11:02:19, two runs. **Mapping:** letter A mapped to RIGHT over MQTT (ACK 11:02:32), no recompile. **Line:** VISIBLE throughout both runs. **Hump:** HUMP events of 9 and 13 mm (run 1) and 12 mm (run 2), line held. **Obstacle stop:** 150 mm box, car at rest at 142 mm and 140 mm (`front=`), target 150 mm, no contact. **Bypass:** box read 216 and 194 mm wide; passed right with 385 mm clearance and left with 1023 mm; back in FOLLOW_LINE about 23 s after each OBSTACLE event. **Barcode turn:** three reads of A, each followed by a right turn at the next junction and an exit on the new line. **Telemetry:** 12 events, each once (3 HUMP, 2 OBSTACLE, 3 BARCODE, 4 TURN); 808 frames, mean 545 ms apart, longest gap 2.0 s. **Stop:** GP21 ended run 1; lifting the car stopped run 2 (lift stop). |

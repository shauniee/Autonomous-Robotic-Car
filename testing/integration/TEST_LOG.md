# Integration — test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)

Tests that two or more subsystems work together through Mission. Both owners agree on the test and sign off the result.

See [testing/README.md](../README.md) for how to run tests and record results.

## Test cases

Define a test here before running it. Test IDs never change.

| ID | What is tested | Requirement / interface | Method | Steps / how | Expected result |
|---|---|---|---|---|---|
| INT-01 | Line following, three laps | S3, S2; M1, MR-26 | manual | 1. Follow a loop with straights and curves at course speed for three laps | No line lost; smooth on video |
| INT-03 | Barcode then left and right turns | S3, S2; M2, M3, MR-28 | manual | 1. Letter mapped to left before a junction. 2. Repeat for right | Turn taken at the next junction; exit on the new line |
| INT-04 | Barcode then straight and U-turn | S3, S2; M3, MR-28 | manual | 1. Letter mapped to straight before a junction. 2. Repeat for U-turn | Straight crosses; U-turn returns along the line |
| INT-05 | Junction or curve with no turn queued | S3, S2; M1 | manual | 1. Drive through a junction and the tightest curve with nothing queued | Car keeps following; no stop |
| INT-07 | Hump while line following | S4, S3, S2; M4, M5 | manual | 1. Hump on a straight, and on a curve if the course has one, while following | Line held; hump counted and height reported |
| INT-08 | Obstacle stop on the line | S5, S2; M6, M8 | manual | 1. Board on the line ahead of the car | Car slows and stops; rest distance taped |
| INT-09 | Bypass and rejoin | S5, S2, S3; M8, M9, MR-30, MR-31 | manual | 1. Box on the line, both sides open | No collision; a side taken; line rejoined |
| INT-10 | Telemetry and events during a run | S1 with all; M10, MR-33 | manual | 1. Run past a barcode, junction, hump and obstacle with the dashboard open. 2. Stop the broker mid-run, then restart | Each event shown once; car keeps driving through the loss; telemetry resumes |

*Method:* **auto** = `TEST_CHECK` in code, result printed on the console;
**manual** = a person observes the result (motor, sensor, dashboard, tape measure).
Each test is shown in one [video](../../video/README.md).

## Results

Add a new row for every run. Never edit or delete old rows.

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
|---|---|---|---|---|---|---|
| YYYY-MM-DD | INT-01 | `abc1234` | | PASS / FAIL | `logs/<ID>/<what>.log`, video link | |
| 2026-11-20 | INT-07 | `880ff16` | Shaun | PASS | `../system/logs/SYS-04/monitor.log`; video: pending | Full course run on battery, fresh boot 11:02:19, two runs over one hump on a straight. Line stayed VISIBLE throughout. Run 2: one HUMP event, 12 mm (11:06:28). Run 1: two HUMP events, 9 mm at 1.17 m travel (nose down, 11:03:15) and 13 mm at 2.47 m (nose up, 11:03:28); the first event's cause is not identified. No curve hump on this course. |
| 2026-11-20 | INT-08 | `880ff16` | Shaun | PARTIAL | `../system/logs/SYS-04/monitor.log`; video: pending | Same run, 150 mm box on the line. Run 1: SCAN entered at front 171 mm, car at rest at 142 mm (11:03:36). Run 2: SCAN entered at 167 mm, at rest at 140 mm (11:06:34). Stop target is 150 mm; no contact. PARTIAL: rest distance read from the sonar, not taped. |

# Motion (S2) — test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)

**Subsystem:** Motion: motors, encoders, wheel PID  ·  **Student ID:** 2503973  ·  **Code:** `app_program/motion/`

See [testing/README.md](../README.md) for how to run tests and record results.

## Test cases

Define a test here before running it. Test IDs never change.

| ID | What is tested | Requirement / interface | Method | Steps / how | Expected result |
|---|---|---|---|---|---|
| S2-U01 | Motor direction and encoder counts | MR-14, MR-19 | manual | 1. Short forward command on each motor. 2. Turn each wheel one revolution by hand, three times | Both wheels drive forward; counts rise and repeat; counts per revolution recorded |
| S2-U02 | 1 m straight | MR-21, D07 | manual | 1. Drive 1 m at course speed, five times. 2. Tape the end point | Distance and lateral error recorded; lateral error at most 10 mm |
| S2-U03 | 90 and 180 degree pivots | MR-22, D07 | manual | 1. Pivot 90 left, 90 right and 180, five each, on a taped square | Heading error recorded; at most 5 degrees |
| S2-U04 | Wheel speed control on the floor | D07 | manual | 1. Run at 100 mm/s with `monitor` saving `WHEEL` traces | Measured speed follows the target without oscillating |
| S2-U05 | Command freshness gate | Motion contract | auto | `motion_test.c` on the TEST image | `[TEST] S2-U05 PASS`; outputs stay inhibited |

*Method:* **auto** = `TEST_CHECK` in code, result printed on the console;
**manual** = a person observes the result (motor, sensor, dashboard, tape measure).
Each test is shown in one [video](../../video/README.md).

## Results

Add a new row for every run. Never edit or delete old rows.

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
|---|---|---|---|---|---|---|
| YYYY-MM-DD | S2-U01 | `abc1234` | | PASS / FAIL | `logs/<ID>/YYYY-MM-DD_….log`, video link | |
| 2026-11-18 | S2-U02 | `2526ba8` (`bench10`, BENCH=2) | Mahesha | PARTIAL | `logs/S2-U02/2026-11-18_monitor.log`, video: pending | Run at 100 mm, not the defined 1 m, on the battery-powered `bench10` image (100 mm/s). Tape: 5/5 runs stopped at 100 mm, 0 mm lateral error. Encoders: 98.2-100.2 mm per run, left-right difference at most 8 counts (~0.3 deg). PARTIAL because the 10 mm lateral limit is set over 1 m. |

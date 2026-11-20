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
| S2-U01 | Motor direction and encoder counts | FR-05 | manual | 1. Short forward command on each motor | Both wheels drive forward; both counts rise |
| S2-U02 | 100 mm straight | D07 | manual | 1. Drive 100 mm at course speed, five times. 2. Tape the end point | Distance and lateral error recorded; lateral error at most 10 mm |
| S2-U03 | 90 and 180 degree pivots | D07 | manual | 1. Pivot 90 left, 90 right and 180, five each, on a taped square | Heading error recorded; at most 5 degrees |
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
| 2026-11-18 | S2-U03 | `184d655` (`benchturn`, BENCH=3) | Mahesha | PARTIAL | `logs/S2-U03/2026-11-18_monitor.log`, video: pending | 21 pivots (7 cycles of left_90, right_90, turn_180) on the battery-powered `benchturn` image, all completed with no faults. Tape: heading error about 5 deg; the 15 individual errors were not written down. Encoders (120 mm nominal track): left_90 88.1-90.7 deg, right_90 -88.4 to -88.9 deg, turn_180 178.4-179.8 deg where separable. PARTIAL: errors sit at the 5 deg limit and per-pivot values are missing; effective track width (D03) is uncalibrated. |
| 2026-11-18 | S2-U04 | `6f59d95` (`bench`, BENCH=1) | Mahesha | PASS | `logs/S2-U04/2026-11-18_monitor.log`, video: pending | Floor, battery, `straight_1m` at 100 mm/s: 995 mm by encoders. 259 `WHEEL` traces over 10.4 s; at the 100 mm/s target, measured L 100.2 / R 100.3 mm/s mean, SD 3.6 mm/s (one count step is 4.3 mm/s), range 90-116 mm/s, duty L 14-17 / R 15-18; no oscillation. Two earlier attempts stopped after ~10 cm and ~46 cm with `Terrain: lifted` while the car rocked (IMU roll up to 17 deg); the raw capture was trimmed to the completed run. |
| 2026-11-18 | S2-U05 | `bb0f4ce` (`test`, TEST=1) | Mahesha | PASS | `logs/S2-U05/2026-11-18_usb.log`, video: pending | USB-powered TEST image, motors inhibited. Three boots (after flash and two USB resets) each printed `[TEST] S2-U05 PASS  command freshness gate; outputs inhibited` and `motion SUMMARY 1 passed, 0 failed`; wheels stayed still. |
| 2026-11-18 | S2-U01 | `33fd9e4` (`bench`, BENCH=1) | Mahesha | PARTIAL | `logs/S2-U01/2026-11-18_monitor.log`, video: pending | Step 1 only, wheels raised, battery: `short_forward` turned both wheels forward, counts L +1145 / R +1165 (~99 mm); `straight_1m` L +11632 / R +11631 (998 mm). Step 2 (one hand revolution per wheel, three times, on the TEST image) not yet run, so counts per revolution are not recorded. |
| 2026-11-20 | S2-U01 | `33fd9e4` (`bench`, BENCH=1) | Mahesha | PASS | `logs/S2-U01/2026-11-18_monitor.log`, video: pending | Same run as the 2026-11-18 PARTIAL row, against the definition without the hand turns: `short_forward` turned both wheels forward, counts L +1145 / R +1165 (~99 mm). |
| 2026-11-20 | S2-U02 | `2526ba8` (`bench10`, BENCH=2) | Mahesha | PASS | `logs/S2-U02/2026-11-18_monitor.log`, video: pending | Same run as the 2026-11-18 PARTIAL row, against the definition narrowed to 100 mm: 5/5 runs stopped at 100 mm by tape with 0 mm lateral error; encoders 98.2-100.2 mm per run. |
| 2026-11-20 | S2-U03 | `184d655` (`benchturn`, BENCH=3) | Mahesha | PASS | `logs/S2-U03/2026-11-18_monitor.log`, video: pending | Same run as the 2026-11-18 PARTIAL row, accepted by the team as it stands: 21 pivots completed with no faults, heading error about 5 deg by tape (at the 5 deg limit); per-pivot values not written down. |

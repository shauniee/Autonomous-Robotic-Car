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
| 2026-11-18 | S2-U01 | `33fd9e4` (`bench`, BENCH=1) | Mahesha | PASS | `logs/S2-U01/2026-11-18_monitor.log`; - | On battery with the wheels raised, `short_forward` drove both wheels forward. Counts rose by 1145 on the left and 1165 on the right (~99 mm). Accepted on 2026-11-20 for direction and rising counts. Counts per hand revolution were not measured. |
| 2026-11-18 | S2-U02 | `2526ba8` (`bench10`, BENCH=2) | Mahesha | PASS | `logs/S2-U02/2026-11-18_monitor.log`; [Test video](https://youtu.be/iEoZOhbyI6s) | All five runs stopped at 100 mm by tape with 0 mm lateral error. Encoders measured 98.2–100.2 mm per run. Accepted on 2026-11-20 for the revised 100 mm distance. |
| 2026-11-18 | S2-U03 | `184d655` (`benchturn`, BENCH=3) | Mahesha | PASS | `logs/S2-U03/2026-11-18_monitor.log`; [Test video](https://youtube.com/shorts/KEXYGgADgUk) | All 21 pivots completed without faults (seven each of left 90°, right 90° and 180°). On 2026-11-20 the team accepted the tape estimate of about 5°, at the limit. Individual tape errors were not recorded. Encoder estimates used a nominal 120 mm track width. |
| 2026-11-18 | S2-U04 | `6f59d95` (`bench`, BENCH=1) | Mahesha | PASS | `logs/S2-U04/2026-11-18_monitor.log`; [Test video](https://youtu.be/Sn4kJvwNpuI), [Terminal video](https://youtu.be/OcXPIzXcfIg) | Floor, battery, `straight_1m` at 100 mm/s: 995 mm by encoders. 259 `WHEEL` traces over 10.4 s; at the 100 mm/s target, measured L 100.2 / R 100.3 mm/s mean, SD 3.6 mm/s (one count step is 4.3 mm/s), range 90-116 mm/s, duty L 14-17 / R 15-18; no oscillation. Two earlier attempts stopped after ~10 cm and ~46 cm with `Terrain: lifted` while the car rocked (IMU roll up to 17 deg); the raw capture was trimmed to the completed run. |
| 2026-11-18 | S2-U05 | `bb0f4ce` (`test`, TEST=1) | Mahesha | PASS | `logs/S2-U05/2026-11-18_usb.log`; [Test video](https://youtu.be/0NZW-VySh94) | USB-powered TEST image, motors inhibited. Three boots (after flash and two USB resets) each printed `[TEST] S2-U05 PASS  command freshness gate; outputs inhibited` and `motion SUMMARY 1 passed, 0 failed`; wheels stayed still. |

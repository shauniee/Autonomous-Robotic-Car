# Obstacle (S5) — test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)

**Subsystem:** Obstacle: sonar, servo scan, profile  ·  **Student ID:** 2502825  ·  **Code:** `app_program/obstacle/`

See [testing/README.md](../README.md) for how to run tests and record results.

## Test cases

Define a test here before running it. Test IDs never change.

| ID | What is tested | Requirement / interface | Method | Steps / how | Expected result |
|---|---|---|---|---|---|
| S5-U01 | Sonar range | MR-17, T08 | manual | 1. Board at 100, 250, 500 and 1000 mm. 2. Open space | Range recorded against a tape; open space reads no echo |
| S5-U02 | Servo centre and limits | MR-18 | manual | 1. `./flash.sh test`. 2. Servo to centre and both sweep limits | Centre faces forward; limits do not stall |
| S5-U03 | Scan profile width | M7, T09 | manual | 1. Scan boxes 100, 150 and 200 mm wide at 300 mm, car still | Width and edges recorded against a tape |
| S5-U04 | Bypass side from the scan | M8, MR-30 | manual | 1. Scan a box centred, left and right of the line, three times each, car still | Side with the shorter edge chosen each time |
| S5-U05 | No echo and out of range | T08 | manual | 1. Aim at open space, a soft object and an angled board | No false obstacle; no stop |

*Method:* **auto** = `TEST_CHECK` in code, result printed on the console;
**manual** = a person observes the result (motor, sensor, dashboard, tape measure).
Each test is shown in one [video](../../video/README.md).

## Results

Add a new row for every run. Never edit or delete old rows.

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
|---|---|---|---|---|---|---|
| YYYY-MM-DD | S5-U01 | `abc1234` | | PASS / FAIL | `logs/<ID>/<what>.log`, video link | |

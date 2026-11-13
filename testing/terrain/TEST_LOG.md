# Terrain (S4) — test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)

**Subsystem:** Terrain: IMU, tilt, lift, humps  ·  **Student ID:** 2500360  ·  **Code:** `app_program/terrain/`

See [testing/README.md](../README.md) for how to run tests and record results.

## Test cases

Define a test here before running it. Test IDs never change.

| ID | What is tested | Requirement / interface | Method | Steps / how | Expected result |
|---|---|---|---|---|---|
| S4-U01 | IMU boot calibration | MR-11, MR-16 | manual | 1. Boot still. 2. Boot while moving. 3. Boot with the IMU disconnected | Still boot calibrates; the others report a calibration fault and refuse to start |
| S4-U02 | Tilt and lift | MR-16, T06 | manual | 1. Tilt the nose up. 2. Start a run and lift the car. 3. Start again and turn it over | Pitch rises; the lift and the turn over each raise `Terrain: lifted` and stop the wheels with a latched stop |
| S4-U03 | Hump count | M4 | manual | 1. Push the car over a 10 mm hump three times, with a pivot between | `humps=` equals the crossings; pivots add none |
| S4-U04 | Hump height | M5, MR-29 | manual | 1. Cross humps from 10 to 30 mm, three passes each | Reported height recorded against the measured height |
| S4-U05 | Hump under the other wheel and in reverse | D11 | manual | 1. Cross a hump with each wheel, forward and reverse | Each counted, with the right side |

*Method:* **auto** = `TEST_CHECK` in code, result printed on the console;
**manual** = a person observes the result (motor, sensor, dashboard, tape measure).
Each test is shown in one [video](../../video/README.md).

## Results

Add a new row for every run. Never edit or delete old rows.

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
|---|---|---|---|---|---|---|
| YYYY-MM-DD | S4-U01 | `abc1234` | | PASS / FAIL | `logs/<ID>/YYYY-MM-DD_….log`, video link | |

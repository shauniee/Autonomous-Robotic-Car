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
| S4-U04 | Hump height | M5, MR-29 | manual | 1. Cross a measured 10 mm hump three times | Reported height recorded against the measured height |
| S4-U05 | Mission stop on lift and flip | MR-16, MR-24 | manual | 1. On the trial image, car READY and not driving, lift it off the floor, set it down, press and release GP21. 2. Repeat, turning the car over | Each raises `Terrain: lifted` and `state=STOPPED`; it stays stopped once flat until GP21, which returns READY |

*Method:* **auto** = `TEST_CHECK` in code, result printed on the console;
**manual** = a person observes the result (motor, sensor, dashboard, tape measure).
Each test is shown in one [video](../../video/README.md).

## Results

Add a new row for every run. Never edit or delete old rows.

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
|---|---|---|---|---|---|---|
| YYYY-MM-DD | S4-U01 | `abc1234` | | PASS / FAIL | `logs/<ID>/<what>.log`, video link | |
| 2026-11-18 | S4-U01 | `2508258` | Darrius | PASS | `logs/S4-U01/usb.log`; no video link yet | `TERRAIN_DIAGNOSTICS=1 ./flash.sh test` on USB. Still boot: `[IMU] calibration passed: samples=100 elapsed_ms=2340` and `[TEST] S4-U01 PASS`. Boot while rocking: `[IMU] calibration failed: timeout samples=0/100 elapsed_ms=5000 stationary=0`. Boot with the IMU disconnected: `[IMU] calibration failed: I2C init failed`; six GP20 presses each gave `[MISSION] start refused` with `start_blocked=1` and `start faults: terrain=3` (IMU fault and calibration failed). The car was also unmapped (`mapped=0`). On the two failed boots the self-check prints `[TEST] S4-U01 FAIL`, as expected. A boot right after flashing, before the steps, latched `Terrain: lifted` and is not in this log. |
| 2026-11-18 | S4-U03 | `2508258` | Darrius | PASS | `logs/S4-U03/usb.log`; no video link yet | `TERRAIN_DIAGNOSTICS=1 ./flash.sh test` on USB, still boot (`[IMU] calibration passed`). One wheel pushed by hand over a 10 mm hump three times, with a hand pivot after each: `[IMU_RAW]` hump count went 0, 1, 2, 3, one per crossing, with `height_valid=1` and rise 11.8, 10.8 and 13.8 mm. The pivots added no hump. Short `ON_HUMP` entries while the car was slid back ended without a count. USB dropped samples 323 to 745 (6.5 to 15.2 s after boot, car at rest before the first crossing; `usb_dropped=26468`). An earlier attempt, not kept, stuck in `ON_HUMP` after the car was tilted by hand with its wheels still and missed its third crossing. |
| 2026-11-18 | S4-U04 | `2508258` | Darrius | PASS | `logs/S4-U04/usb.log`; no video link yet | `TERRAIN_DIAGNOSTICS=1 ./flash.sh test` on USB, still boot (`[IMU] calibration passed`). One wheel pushed by hand over a measured 10 mm hump three times: each crossing counted with `height_valid=1`, reported rise 11.5, 13.6 and 13.5 mm against 10 mm measured (+1.5, +3.6, +3.5 mm). Two `LIFTED` blips while the car was moved between passes added no hump. Run before the S4-U04 definition changed to one 10 mm hump (MR-29 now uses only 10 mm humps). |
| 2026-11-18 | S4-U05 | `a67d645` | Darrius | PASS | `logs/S4-U05/monitor.log`; no video link yet | `./flash.sh trial` on battery, car READY with no run started (firmware unchanged since `2508258`). Lift: telemetry went to `state=STOPPED` with `faults: Terrain: lifted`; once set down the fault cleared but the car stayed STOPPED until GP21, then READY (17:02:43 to 17:02:51). Turn over: `state=STOPPED` with `Terrain: lifted`; set back on its wheels, the fault cleared and GP21 returned READY (17:02:56 to 17:03:02). GP21 presses are from the steps; telemetry does not report buttons. An earlier bench run on the TEST image, not kept, latched the stop but GP21 could not clear it, because that image keeps the motors inhibited. |

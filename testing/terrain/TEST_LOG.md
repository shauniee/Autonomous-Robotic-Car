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
| S4-U01 | IMU boot calibration | FR-11 | manual | 1. Boot still. 2. Boot while moving. 3. Boot with the IMU disconnected | Still boot calibrates; the others report a calibration fault and refuse to start |
| S4-U02 | Hump count and height | M4, M5 | manual | Cross one measured 10 mm hump three times by hand and capture `usb.log` | `humps=` increments once per crossing and ends at 3. Record the calibrated USB height estimate for each crossing against the 10 mm reference. |
| S4-U03 | Mission stop on lift and flip | NFR-07 | manual | 1. On the trial image, car READY and not driving, lift it off the floor, set it down, press and release GP21. 2. Repeat, turning the car over | Each raises `Terrain: lifted` and `state=STOPPED`; it stays stopped once flat until GP21, which returns READY |

*Method:* **auto** = `TEST_CHECK` in code, result printed on the console;
**manual** = a person observes the result (motor, sensor, dashboard, tape measure).
Each test is shown in one [video](../../video/README.md).

## Results

The table lists the latest result for each test ID. Earlier runs are in Git history.

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
|---|---|---|---|---|---|---|
| 2026-11-22 | S4-U01 | `df6dd4b` (`test`) | Darrius | PARTIAL | `logs/S4-U01/usb.log`; [Test video](https://youtube.com/shorts/spz0Khsx9uk), [Terminal video](https://youtu.be/2H9F70z85SY) | Three boots calibrated and two timed out. Disconnected-IMU and GP20 start-refusal checks are pending. |
| 2026-11-22 | S4-U02 | `df6dd4b` (`test`) | Darrius | PASS | `logs/S4-U02/usb.log`; [Test video](https://youtube.com/shorts/S-PKnSbOT6c), [Terminal video](https://youtu.be/ww6Ak2KIjOs) | Counted all three crossings. Height estimates were 11.1, 8.3 and 13.3 mm against a 10 mm reference. |
| 2026-11-22 | S4-U03 | `df6dd4b` (`trial`) | Darrius | PARTIAL | `logs/S4-U03/monitor.log`; [Test video](https://youtube.com/shorts/vWSq1_WDSps), [Terminal video](https://youtu.be/YC7Mm6QmjDA) | Mission stayed STOPPED after the lift fault cleared and later returned to READY. Video confirmation of the lift, flip and GP21 presses is pending. |

# Line and barcode (S3) — test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)

**Subsystem:** Line IR and barcode  ·  **Student ID:** 2501552  ·  **Code:** `app_program/line_barcode/`

See [testing/README.md](../README.md) for how to run tests and record results.

## Test cases

Define a test here before running it. Test IDs never change.

| ID | What is tested | Requirement / interface | Method | Steps / how | Expected result |
|---|---|---|---|---|---|
| S3-U01 | Line IR levels | MR-12, D04 | manual | 1. Hold each mounted line sensor (GP26, GP17) over black, white and the line edge | Black and white read correctly on both |
| S3-U02 | Barcode IR level | MR-13, D04 | manual | 1. Slide black and white under the barcode sensor (GP27) with `[BCPIN]` on | Level flips and edges count; pot setting recorded |
| S3-U03 | All 26 letters | M2, MR-27 | manual | 1. Drive over each printed letter at course speed | Each letter decodes once and matches the print |
| S3-U04 | Reverse direction and flipped print | M2, MR-27 | manual | 1. Drive over letters in the reverse direction. 2. Repeat with the print turned 180 degrees | Same letter decoded |
| S3-U05 | Read rate at course speed | M2, T04 | manual | 1. Pass one letter ten times at course speed | Reads out of passes recorded; no wrong letter |

*Method:* **auto** = `TEST_CHECK` in code, result printed on the console;
**manual** = a person observes the result (motor, sensor, dashboard, tape measure).
Each test is shown in one [video](../../video/README.md).

## Results

Add a new row for every run. Never edit or delete old rows.

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
|---|---|---|---|---|---|---|
| YYYY-MM-DD | S3-U01 | `abc1234` | | PASS / FAIL | `logs/<ID>/<what>.log`, video link | |
| 2026-11-18 | S3-U01 | `893c6c8` | Nicholas | PASS | `logs/S3-U01/usb.log`; no video link yet | `[TEST] S3-U01 PASS  line IR sample valid, no IR_INVALID fault` at power-up on the TEST image (USB). On the bench, `[IR] black=` showed `0/0` over white, `1/0` with black under the left sensor (GP26) only, `0/1` with black under the right sensor (GP17) only and `1/1` with both over black: each sensor reads black and white on its own. WiFi was not joined (`SSID not found`); not needed for this test. |

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
| S3-U01 | Line IR levels | D04 | manual | 1. Hold each mounted line sensor (GP26, GP17) over black, white and the line edge | Black and white read correctly on both |
| S3-U02 | Barcode IR level | D04 | manual | 1. Slide black and white under the barcode sensor (GP27) with `[BCPIN]` on | Level flips and edges count |
| S3-U03 | All 26 letters | — | manual | 1. Drive over each printed letter at course speed | Each letter decodes once and matches the print |
| S3-U04 | Flipped print | — | manual | 1. Turn one printed letter 180 degrees. 2. Drive over it in the normal direction | Same letter decoded |
| S3-U05 | Read rate at course speed | T04 | manual | 1. Pass one letter three times at course speed | Reads out of passes recorded; no wrong letter |

*Method:* **auto** = `TEST_CHECK` in code, result printed on the console;
**manual** = a person observes the result (motor, sensor, dashboard, tape measure).
Each test is shown in one [video](../../video/README.md).

## Results

Add a new row for every run. Never edit or delete old rows.

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
|---|---|---|---|---|---|---|
| 2026-11-18 | S3-U01 | `893c6c8` | Nicholas | PASS | `logs/S3-U01/usb.log`; [Test video](https://youtu.be/FnCqoq3C2mk) | `[TEST] S3-U01 PASS  line IR sample valid, no IR_INVALID fault` at power-up on the TEST image (USB). On the bench, `[IR] black=` showed `0/0` over white, `1/0` with black under the left sensor (GP26) only, `0/1` with black under the right sensor (GP17) only and `1/1` with both over black: each sensor reads black and white on its own. WiFi was not joined (`SSID not found`); not needed for this test. |
| 2026-11-18 | S3-U02 | `ce16c4f+` | Nicholas | PASS | `logs/S3-U02/usb.log`; [Terminal video](https://youtu.be/16t21lRjw1w), [Test video](https://youtube.com/shorts/OAcXRysHmjQ) | `BARCODE_DIAGNOSTICS=1 ./flash.sh test` on USB, with the edge IRQ armed at init by the uncommitted edge-count fix (`fix(line_barcode): count barcode edges on the TEST diagnostics image`, kept on its own branch for main). Sliding black then white under the barcode sensor (GP27) three times flipped `[BCPIN] level=` between 0 and 1 (33 changes after boot), and `edges=` rose from 0 to 6666. Each slow slide counted tens to hundreds of IRQ edges, which is sensor chatter near the switching point; the counter includes chatter that the decoder discards. |
| 2026-11-18 | S3-U03 | `5c0d4e0` | Nicholas | PASS | `logs/S3-U03/monitor.log`; [Test video](https://youtu.be/Z7fyduXBGNg), [Terminal video](https://youtu.be/wuxQHhPDDg4) | On battery at course speed, the final A–Z sequence took 34 GP20 starts. Of 28 passes over a print, 26 produced a read, one for each letter in order. One pass each missed C and I. Every read matched the print, with no duplicate reads per pass. Three earlier practice reads precede the sequence in the log. |
| 2026-11-18 | S3-U04 | `5c0d4e0` | Nicholas | PASS | `logs/S3-U04/monitor.log`; [Test video](https://youtube.com/shorts/-9bpd8Z4s1g), [Terminal video](https://youtu.be/C3tNZB74D6c) | The Z print was turned 180° and the car crossed it twice in its normal driving direction. Both passes decoded Z. |
| 2026-11-18 | S3-U05 | `5c0d4e0` | Nicholas | PASS | `logs/S3-U05/monitor.log`; [Test video](https://youtube.com/shorts/c717WRrZeYQ), [Terminal video](https://youtu.be/Vnk85cuwu1A) | `BARCODE_DIAGNOSTICS=1 ./flash.sh trial` on battery, letter Q (mapped to LEFT) on the course. 3 passes, not the ten in the steps: one GP20 start looped the course three times. Each pass gave one `EVENT BARCODE: letter Q`, 3 reads out of 3 passes, with no wrong letter, and each read was followed by the queued left turn (`NAVIGATE`). Six earlier Q reads in practice runs are not in this log. |

**Note on the 2026-11-18 S3-U05 row (added 2026-11-22):** the steps now ask for three passes, so the remark "not the ten in the steps" is out of date; three passes meet the steps.

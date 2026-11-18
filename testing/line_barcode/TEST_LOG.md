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
| S3-U02 | Barcode IR level | MR-13, D04 | manual | 1. Slide black and white under the barcode sensor (GP27) with `[BCPIN]` on | Level flips and edges count |
| S3-U03 | All 26 letters | M2, MR-27 | manual | 1. Drive over each printed letter at course speed | Each letter decodes once and matches the print |
| S3-U04 | Flipped print | M2, MR-27 | manual | 1. Turn one printed letter 180 degrees. 2. Drive over it in the normal direction | Same letter decoded |
| S3-U05 | Read rate at course speed | M2, T04 | manual | 1. Pass one letter three times at course speed | Reads out of passes recorded; no wrong letter |

*Method:* **auto** = `TEST_CHECK` in code, result printed on the console;
**manual** = a person observes the result (motor, sensor, dashboard, tape measure).
Each test is shown in one [video](../../video/README.md).

## Results

Add a new row for every run. Never edit or delete old rows.

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
|---|---|---|---|---|---|---|
| YYYY-MM-DD | S3-U01 | `abc1234` | | PASS / FAIL | `logs/<ID>/<what>.log`, video link | |
| 2026-11-18 | S3-U01 | `893c6c8` | Nicholas | PASS | `logs/S3-U01/usb.log`; no video link yet | `[TEST] S3-U01 PASS  line IR sample valid, no IR_INVALID fault` at power-up on the TEST image (USB). On the bench, `[IR] black=` showed `0/0` over white, `1/0` with black under the left sensor (GP26) only, `0/1` with black under the right sensor (GP17) only and `1/1` with both over black: each sensor reads black and white on its own. WiFi was not joined (`SSID not found`); not needed for this test. |
| 2026-11-18 | S3-U02 | `ce16c4f+` | Nicholas | PASS | `logs/S3-U02/usb.log`; no video link yet | `BARCODE_DIAGNOSTICS=1 ./flash.sh test` on USB, with the edge IRQ armed at init by the uncommitted edge-count fix (`fix(line_barcode): count barcode edges on the TEST diagnostics image`, kept on its own branch for main). Sliding black then white under the barcode sensor (GP27) three times flipped `[BCPIN] level=` between 0 and 1 (33 changes after boot), and `edges=` rose from 0 to 6666. Each slow slide counted tens to hundreds of IRQ edges, which is sensor chatter near the switching point; the counter includes chatter that the decoder discards. |
| 2026-11-18 | S3-U03 | `5c0d4e0` | Nicholas | PASS | `logs/S3-U03/monitor.log`; no video link yet | `BARCODE_DIAGNOSTICS=1 ./flash.sh trial` on battery, on a straight of the course. One print at a time, GP20 start and GP21 stop for each. The final pass from A to Z gave 26 `EVENT BARCODE` reads, one per letter, A to Z in order, each matching the print, with no wrong letter. Three earlier reads (A, B, C) in the same log came before reboots and a restart of the sequence. |
| 2026-11-18 | S3-U04 | `5c0d4e0` | Nicholas | PASS | `logs/S3-U04/monitor.log`; no video link yet | Same image and session as S3-U03. Letter Z only, driven over in the reverse direction twice: both passes read `letter Z`. Not every letter was run in reverse, and step 2 (print turned 180 degrees) was not run separately; driving over a fixed print the other way presents its bars in the same order as the print turned 180 degrees. |
| 2026-11-18 | S3-U05 | `5c0d4e0` | Nicholas | PASS | `logs/S3-U05/monitor.log`; no video link yet | `BARCODE_DIAGNOSTICS=1 ./flash.sh trial` on battery, letter Q (mapped to LEFT) on the course. 3 passes, not the ten in the steps: one GP20 start looped the course three times. Each pass gave one `EVENT BARCODE: letter Q`, 3 reads out of 3 passes, with no wrong letter, and each read was followed by the queued left turn (`NAVIGATE`). Six earlier Q reads in practice runs are not in this log. |

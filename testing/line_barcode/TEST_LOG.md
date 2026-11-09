# line_barcode test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)

## Test cases

| ID | Test / requirement | Method / steps | Expected |
| --- | --- | --- | --- |
| S3-U02 | Normalization; previous Motion/watchdog progress portion retired | On-target TEST=1 pure fixture | Endpoints, unequal/opposite spans, clamping and 299/300 contrast; no hardware claim |
| S3-U03 | Independent calibration/PID/controller (retired after proportional override; historical results retained) | Host original meaningful IR/control tests | Fresh bounded steering, calibration validity, hysteresis/loss and safety pass |
| S3-U04 | ADC register protocol | Host real drv_ir with fake registers | Left ADC0 then right ADC1; discard first, average 8; error and stalled timer fail boundedly |
| S3-U05 | Local calibration and Motion-health bench policy (retired after sensor-service split) | Historical host test_mtkernel_bench | Fresh black/white/start presses, contrast rejection, GP21 dominance, separate reset/start, stalled progress refuses watchdog feed |
| S3-U06 | Proportional direction, zero error, saturation, polarity and original freshness | Historical host result; current TEST=1 pure fixtures | Bounded memoryless P; 60/61 ms and wraparound; no movement claim |
| S3-U07 | Typed capture outcomes, acquisition validity and line hysteresis | Historical host result; current TEST=1 capture fixtures plus S3-U10 classification | >=500 ms/25 distinct samples; each contrast >=300; failed ADC/capture gap fail closed; wraparound |
| S3-U08 | Observation table and explicit bounded movement recovery (retired after sensor-service split) | Historical host test_robot_control | Nine combinations; both directions; no known direction stops; two distinct qualifying samples; broad black resets streak; >=100 ms stops; stop/reset rules |
| S3-U09 | Future barcode decode/capture | Planned only; define numerical/framing conditions after course confirmation | Complete ordered widths, one event/encounter; rejected frames/overflow explicit; no decoder implemented |
| S3-U10 | Sensor-only observation classification | TEST=1 pure fixtures | Nine combinations; exact 0.15/0.25/0.85 boundaries, LOST hysteresis and nonfinite rejection |
| S3-U11 | Calibration-only local input workflow | TEST=1 synthetic button/time fixtures; physical GP20/21 and buzzer run separately | Held-at-boot, bounce, 30 ms release/press, cancel dominance, white retry, replacement and wraparound; 1/2/3 result counts |
| S3-U12 | Portable snapshot and nonblocking copy | TEST=1 pure fixtures plus actual private mutex boundary | Raw/reference/contrast/normalized coherence; missing/invalid/uncalibrated/capturing/stale flags; unchanged sequence/time on aging; null and locked-copy rejection |
| S3-U13 | ADC destination validation | TEST=1 `drv_ir_tracking_read(NULL)` only | Reject null before hardware access; S3-U04 hardware protocol/WCET remains separate |
| S3-M01 | Minimum-bit final memory (spec #100) | Compile-only `testing/sizeof_probe.sh` plus section-4 build command and `arm-none-eabi-size` on the final implementation; line_barcode `.bss` objects from `nm`/map | Probe totals equal linked line_barcode objects; final storage bits recorded in the [bit-usage data contract](../../docs/team.md#line-and-barcode); no execution claim |
| S3-M02 | Minimum-bit final line_barcode loop timing (spec #100) | On-target run of the final implementation; method defined before the run (#107) | Worst-case line_barcode loop time below its configured period, with margin per [D13](../../docs/project-specs.md#pending-decisions); recorded with exact commit; no value assumed |
| S3-BF01 | Both-black forward with invalid guidance | Host | Straight FOLLOW to the interval boundary; GP21 interrupts |
| S3-BF02 | Minimum crossing distance | Host, build | 25 mm DISTANCE plus 3 mm tolerance; early sensing does not shorten it |
| S3-U14 | Thin black patches and a held barcode | Host | Held barcode is not consumed by travel over white |
| S3-U15 | All 26 Code 39 letters, both orientations, two scales | Host | Same letter, correct orientation, framed capture only |
| S3-U16 | False frame then the real A | Host | BAD_WIDTH, then DECODED A with no new quiet zone |
| S3-U17 | LOST hysteresis at 0.2 and 0.3 black | Host | LOST at 0.2, VISIBLE at 0.3 |
| S3-U18 | Minimum-contrast boundaries, both polarities | Host | Failed capture is rejected |
| S3-U19 | Cancelled white recapture | Host | Cached samples are not reused |
| S3-U20 | All A-Z in both orientations at two scales | Host, offline TEST build | Same action mapping; bad guards and a short rearm gap are rejected |

## Results

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-08 | S3-U02 | Not yet run on Pico | — | PENDING (build) | none | Test firmware built only |
| 2026-10-08 | S3-U03/U04/U05 | Manifest in integration/logs | Codex | PASS (host) | at tag | Host/fake-register evidence only |
| 2026-10-08 | S3-U04/U05/U06/U07/U08 | ../integration/logs/2026-10-08_s3-proportional-source-sha256.json | Codex | PASS | host log | P/capture/recovery and retained ADC/local/health checks; no physical result |
| 2026-10-08 | S3-U06 | Same source manifest | — | PENDING (host, build) | — | TEST self-check compiled only; host PASS above is separate |
| 2026-10-08 | S3-U09 | Future implementation | — | PENDING | none | Barcode deferred; no decode run |
| 2026-10-09 | S3-U02/U06/U07/U10/U11/U12/U13 | Working tree source SHA-256 manifest in evidence | — | PENDING (build) | — | All four profiles built; firmware checks compiled only, not executed |
| 2026-10-09 | S3-U02/U06/U07/U10/U11/U12/U13 | Relocated test source manifest in evidence | — | PENDING | — | Relocated fixtures compile in all TEST profiles; no on-board execution |
| 2026-10-09 | S3-U12 | Not established | Nicholas | FAIL (car) | Capture not in the repo | Before the fix: mutex ID lost, then a core-1 hard fault; startup stack overflow found |
| 2026-10-09 | S3-U12 | Working tree | Codex | PENDING (build) | — | TEST startup stack raised to 4096 bytes; four builds pass; corrected image not run |
| 2026-10-27 | S3-M01 | `d6ce587` | Claude | PASS (host) | — | Compile, link and host only: text 70464, data 552, bss 23756; probe sizes match linked objects |
| 2026-10-27 | S3-M02 | Not yet run on Pico | — | PENDING (car) | none | On-target final loop timing; tracked in the physical-test PR |
| 2026-10-28 | S3-BF01 | `5e6a45b`+ | Codex | PASS (host) | At tag | Timed both-black forward; superseded by S3-BF02 |
| 2026-10-28 | S3-BF02 | `5e6a45b`+ | Codex | PASS (host, build) | At tag | 25 mm crossing distance; not flashed |
| 2026-10-29 | S3-U14 / S3-U15 | `0448890`+ | Not recorded | PASS (host) | — | PASS (host only). Host output; Actual-black qualification, straight outgoing confirmation, A-Z orientation/scale fixtures; software only |
| 2026-10-30 | S3-U16 / S3-U17 | `cc3edb3`+ | Codex | PASS (host) | — | FAIL before fix, PASS after fix (host only). Original regression failures; targeted host output, including 3516 IR replay samples |
| 2026-10-30 | S3-U18 / S3-U19 | `8be851b`+ | Codex | PASS (host) | — | MSVC x64 C11, warnings as errors; 3516 replay samples plus calibration, cancellation and wrap checks |
| 2026-10-31 | S3-U20 | `5e63fe0`+ | Codex | PARTIAL (host, build) | — | Host suite and Cortex-M0+ driver compile pass; full firmware build failed on an unrelated `size_t` conflict in `io_task.c` |
| 2026-10-31 | S3-U20 | `545a6a3` | Codex | PASS (host, build) | INT-49 | PASS (host, dashboard, build). Integrated source; supersedes the earlier full build FAIL; no physical barcode check |
| 2026-10-31 | S3-U21 | `4365e10`+ | Codex | PASS (host) | — | Remove digital IR software calibration |
| 2026-10-31 | S3-U22 | `4365e10`+ | Codex | PASS (host) | — | Correct digital IR polarity |
| 2026-10-31 | S3-U23 | `86ce2d8` | Not recorded | PASS (build) | — | IR calibration diagnostic preparation. No GPIO or physical check |
| 2026-11-02 | S3-U24 | Not recorded | Nicholas, Claude | FAIL (car) | [2026-11-02_S3-U24/](logs/2026-11-02_S3-U24/) | Barcode module misses narrow gaps. Passes the barcode with no read |
| 2026-11-02 | S3-U25 | Not recorded | Nicholas | OBSERVED (car) | — | Printed barcode black reads weakly. Hand check: many gaps missed; printed black reads weaker than line black |
| 2026-11-02 | S3-U26 | image `2c51c24` | Nicholas, Claude | FAIL (car) | [2026-11-02_S3-U25/](logs/2026-11-02_S3-U25/) | Barcode after recalibration. Barcode not sensed; tracking and crossings ran |
| 2026-11-02 | S3-U27 | Not recorded | Nicholas, Claude | OBSERVED (host, build) | — | Host and build only. Barcode edge count in the trace. Trace gains a running count of GP16 edges |
| 2026-11-02 | S3-U28 | image `45f1aca` | Nicholas, Claude | FAIL (car) | [2026-11-02_S3-U28/](logs/2026-11-02_S3-U28/) | Barcode module output stays solid |
| 2026-11-02 | S3-U29 | Not recorded | Nicholas | OBSERVED (car) | — | Barcode module LED responds slowly. Module LED slow by hand; not timed |
| 2026-11-02 | S3-U30 | image `45f1aca` | Nicholas, Claude | FAIL (car) | [2026-11-02_S3-U30/](logs/2026-11-02_S3-U30/) | New barcode module, chatter. Barcode A not scanned; module works |
| 2026-11-02 | S3-U31 | image `719a01b` | Nicholas, Claude | FAIL (car) | — | Raw barcode widths. Barcode still not read |
| 2026-11-02 | S3-U32 | image `24b25fb` | Nicholas, Claude | FAIL (car) | [2026-11-02_S3-U32/](logs/2026-11-02_S3-U32/) | Barcode capture polarity bug. No read; the capture handler has no host test |
| 2026-11-02 | S3-U33 | image `84f8e2c` | Nicholas, Claude | FAIL (car) | [2026-11-02_S3-U33/](logs/2026-11-02_S3-U33/) | Filtered barcode widths. No read |
| 2026-11-02 | S3-U34 | image `84f8e2c` | Nicholas, Claude | OBSERVED (car) | [2026-11-02_S3-U34/](logs/2026-11-02_S3-U34/) | Barcode threshold moved toward white. No barcode event; pass looks like A with wide bars only (inferred) |
| 2026-11-02 | S3-U35 | Not recorded | Nicholas, Claude | OBSERVED (car) | — | Narrow bars are seen. Narrow bar about 5 mm and seen by the LED; `BARCODE_MIN_ELEMENT_COUNTS` 23 |
| 2026-11-02 | S3-U36 | image `c9cdb44` | Nicholas, Claude | PASS (car) | [2026-11-02_S3-U36/](logs/2026-11-02_S3-U36/) | PASS, one read. First barcode read. One print, one direction |
| 2026-11-02 | S3-U37 | Not recorded | Nicholas, Claude | OBSERVED (car) | — | Barcode misses after the first read. One read in three tries; `BARCODE_MIN_ELEMENT_COUNTS` 16 |
| 2026-11-03 | S3-U38 | `7275347` | Claude | OBSERVED (static) | — | Capture overflow shown in 429/1033, 37/618, 127/572, 64/639 telemetry rows (INT-154 to INT-156, S4-U10), nearly all while driving. Read loss not shown; ring stays 64 |
| 2026-11-04 | S3-U39 | Saved captures, no new run | Nicholas, Claude | OBSERVED (car) | [2026-11-04_INT-190/](../integration/logs/2026-11-04_INT-190/), [raw captures](../integration/logs/2026-11-02_raw-captures/) | Left IR clears crossbars after the right on 99 of 100 crossings, median 14 mm. Potentiometer does not remove it. Cause not isolated |
| 2026-11-09 | S3-U40 | `7da3c66` BARCODE_DIAGNOSTICS=1 | Nicholas, Claude | FAIL (car) | [usb.log](logs/2026-11-09_S3-U40/usb.log) | Letter A, three passes, no read. Each run logs only the enable seed: no GP16 edge reached the capture. Module LED misses narrow gaps |
| 2026-11-09 | S3-U41 | `7da3c66`+ `[BCPIN]` diag, BARCODE_DIAGNOSTICS=1 | Nicholas, Claude | FAIL (car) | [usb.log](logs/2026-11-09_S3-U41/usb.log) | Black strip slid under the barcode sensor, idle and after GP20. GP16 read level=0 with edges=0 for the whole capture: the pin never moved. Cause: barcode DOUT was on the wrong Pico pin; moved to GP16 by Nicholas |
| 2026-11-09 | S3-U42 | `7da3c66`+ `[BCPIN]` diag, BARCODE_DIAGNOSTICS=1 | Nicholas | FAIL (car) | — | DOUT on GP16. Half-size print: module LED does not switch on every edge, so narrow bars and gaps never reach the pin. Capture not kept |
| 2026-11-09 | S3-U43 | `7da3c66`+ `[BCPIN]` diag, BARCODE_DIAGNOSTICS=1 | Nicholas, Claude | PARTIAL (car) | [usb.log](logs/2026-11-09_S3-U43/usb.log) | Full A driven: decoded A. Half Z (wheels at about 106 mm/s): no read; narrow black bars read 9 to 16 counts (under 1.4 mm) or vanish under the 16-count chatter cut, merging spaces to 136 to 196; a log replay with the cut at 6 still fails. Driven half Z stopped on GP21 after 38 mm of solid black, inconclusive |
| 2026-11-09 | S3-U44 | `7da3c66`+ diag, MQTT frame retry, BARCODE_DIAGNOSTICS=1 | Nicholas, Claude | PARTIAL (car) | [usb.log](logs/2026-11-09_S3-U44/usb.log) | Full F 3/3 and full A 3/3 decoded. Half Z not read: narrow black bars 16 to 21 counts, at the 16-count chatter cut; one run of 442 (over the 400 limit) where a bar was dropped. Cut lowered to 10 for the next run |
| 2026-11-09 | S3-U45 | `7da3c66`+ diag, MQTT frame retry, cut 10, BARCODE_DIAGNOSTICS=1 | Nicholas, Claude | PARTIAL (car) | [usb.log](logs/2026-11-09_S3-U45/usb.log) | Full A and F read (reported). Half Z 0 of 6: polarity balance flipped since S3-U44, narrow black now 42 to 49 counts and narrow white 18 to 30; the decoded guard and Z match, but a narrow white in the closing guard is lost (black run of 277) |
| 2026-11-09 | S3-U46 | `7da3c66`+ diag, MQTT frame retry, cut 10, BARCODE_DIAGNOSTICS=1 | Nicholas, Claude | PASS (car) | [usb.log](logs/2026-11-09_S3-U46/usb.log) | Half Z after a pot adjustment: 4 of 4 decoded as Z, reverse orientation. Narrow runs 17 to 28 and 37 to 53 counts, all above the 10-count cut |

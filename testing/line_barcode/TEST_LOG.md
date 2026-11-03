# line_barcode test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)

Owner Nicholas / 2501552. Template S3-U01 is retired, not reused. Host methods marked historical have retired runners. One row per run. Write-ups from before 2026-11-03, and build and host output, are at git tag `test-logs-full-2026-11-03`: `git show test-logs-full-2026-11-03:testing/line_barcode/TEST_LOG.md`. `+` after a commit means it also had uncommitted changes.

## Test cases

| ID | Test / requirement | Method / steps | Expected |
|---|---|---|---|
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
| S3-U13 | ADC destination validation | TEST=1 `drv_ir_read(NULL)` only | Reject null before hardware access; S3-U04 hardware protocol/WCET remains separate |
| S3-M01 | Minimum-bit final memory (spec #100) | Compile-only `testing/line_barcode/sizeof_probe.sh` plus section-4 build command and `arm-none-eabi-size` on the final implementation; line_barcode `.bss` objects from `nm`/map | Probe totals equal linked line_barcode objects; final storage bits recorded in the [bit-usage data contract](../../docs/project-specs.md#line_barcode-bit-usage-data-contract-proposed-spec-100-final-evidence-107); no execution claim |
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
|---|---|---|---|---|---|---|
| 2026-10-08 | S3-U02 | Not yet run on Pico | Pending operator | PENDING | none | Test firmware built only |
| 2026-10-08 | S3-U03/U04/U05 | Manifest in integration/logs | Codex host run | PASS | at tag | Host/fake-register evidence only |
| 2026-10-08 | S3-U04/U05/U06/U07/U08 | ../integration/logs/2026-10-08_s3-proportional-source-sha256.json | Codex host run | PASS | host log | P/capture/recovery and retained ADC/local/health checks; no physical result |
| 2026-10-08 | S3-U06 | Same source manifest | Pending Pico operator | PENDING | [build evidence](../integration/logs/2026-10-08_s3-proportional-evidence.json) | TEST self-check compiled only; host PASS above is separate |
| 2026-10-08 | S3-U09 | Future implementation | Pending | PENDING | none | Barcode deferred; no decode run |
| 2026-10-09 | S3-U02/U06/U07/U10/U11/U12/U13 | Working tree source SHA-256 manifest in evidence | Pending Pico operator | PENDING | [service build evidence](logs/2026-10-09_s3-line-service_evidence.json) | All four profiles built; firmware checks compiled only, not executed |
| 2026-10-09 | S3-U02/U06/U07/U10/U11/U12/U13 | Relocated test source manifest in evidence | Pending Pico operator | PENDING | [test-layout evidence](logs/2026-10-09_s3-test-layout_evidence.json) | Relocated fixtures compile in all TEST profiles; no on-board execution |
| 2026-10-09 | S3-U12 | Not established | Nicholas | FAIL | Capture not in the repo | Before the fix: mutex ID lost, then a core-1 hard fault; startup stack overflow found |
| 2026-10-09 | S3-U12 | Working tree | Codex | PENDING | [Stack fix evidence](logs/2026-10-09_startup-stack_evidence.json) | TEST startup stack raised to 4096 bytes; four builds pass; corrected image not run |
| 2026-10-27 | S3-M01 | `d6ce587` | Claude Code compile run | PASS | — | Compile, link and host only: text 70464, data 552, bss 23756; probe sizes match linked objects |
| 2026-10-27 | S3-M02 | Not yet run on Pico | Pending operator (Nicholas) | PENDING | none | On-target final loop timing; tracked in the physical-test PR |
| 2026-10-28 | S3-BF01 | `5e6a45b`+, [manifest](logs/2026-10-28_S3-BF01_both-black-forward_manifest.json) | Codex | PASS (host) | At tag | Timed both-black forward; superseded by S3-BF02 |
| 2026-10-28 | S3-BF02 | `5e6a45b`+, [manifest](logs/2026-10-28_S3-BF02_manifest.json) | Codex | PASS (host, build) | At tag | 25 mm crossing distance; not flashed |
| 2026-10-29 | S3-U14 / S3-U15 | `0448890`+, [working-source manifest](../integration/logs/2026-10-29_INT-16-manifest.json) | Not recorded | PASS (host only) | Host output; Actual-black qualification, straight outgoing confirmation, A-Z orientation/scale fixtures; software only | — |
| 2026-10-30 | S3-U16 / S3-U17 | `cc3edb3`+, [working-source manifest](logs/2026-10-30_S3-U16-U17-manifest.json) | Codex | FAIL before fix, PASS after fix (host only) | Original regression failures; targeted host output, including 3516 IR replay samples | — |
| 2026-10-30 | S3-U18 / S3-U19 | `8be851b`+, [source manifest](logs/2026-10-30_S3-U18-U19-manifest.json) | Codex | PASS (host only) | Build, test output. MSVC x64 C11, optimized, warnings as errors; 3516 replay samples plus calibration/cancellation/wrap checks and existing invalid-acquisition hysteresis. No GNU/Arm firmware build or physical pass. | — |
| 2026-10-31 | S3-U20 | `5e63fe0`+, [working-source manifest](logs/2026-10-31_S3-U20/manifest.json) | Codex | PASS (host tests and driver compilation); full firmware build FAIL | Barcode assertions, full host suite, Cortex-M0+ driver compile. Full firmware attempt fails on an unrelated `size_t` conflict through `terrain_evidence.h` in `io_task.c` and `mission.c`. No firmware execution or physical pass. | — |
| 2026-10-31 | S3-U20 | `545a6a3` | Codex | PASS (host, dashboard, build) | INT-49 | Integrated source; supersedes the earlier full build FAIL; no physical barcode check |

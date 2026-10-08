# S3 IR bench test log

Owner Nicholas / 2501552. Template example S3-U01 retired, not reused.

## 1. Test cases

| ID | Test / requirement | Method / steps | Expected |
|---|---|---|---|
| S3-U02 | Normalization/progress gate | On-target TEST=1 self-check | `[TEST] S3-U02 PASS`; acquisition/physical tests separate |
| S3-U03 | Independent calibration/PID/controller (retired after proportional override; historical results retained) | Host original meaningful IR/control tests | Fresh bounded steering, calibration validity, hysteresis/loss and safety pass |
| S3-U04 | ADC register protocol | Host real drv_ir with fake registers | Left ADC0 then right ADC1; discard first, average 8; error and stalled timer fail boundedly |
| S3-U05 | Local calibration and health policy | Host test_mtkernel_bench | Fresh black/white/start presses, contrast rejection, GP21 dominance, separate reset/start, stalled progress refuses watchdog feed |
| S3-U06 | Proportional direction, zero error, saturation, polarity and original freshness | Host test_ir_tracking; independent TEST=1 pure self-check | Bounded memoryless P; separate host/on-target results |
| S3-U07 | Typed capture outcomes, acquisition validity and line hysteresis | Host test_ir_tracking | >=500 ms/25 distinct samples; each contrast >=300; failed ADC/capture gap fail closed; wraparound |
| S3-U08 | Observation table and explicit bounded recovery | Host test_robot_control | Nine combinations; both directions; no known direction stops; two distinct qualifying samples; broad black resets streak; >=100 ms stops; stop/reset rules |
| S3-U09 | Future barcode decode/capture | Planned only; define numerical/framing conditions after course confirmation | Complete ordered widths, one event/encounter; rejected frames/overflow explicit; no decoder implemented |

## 2. Results

| Date | ID | Source | Tester | Result | Evidence | Notes |
|---|---|---|---|---|---|---|
| 2026-10-08 | S3-U03/U04/U05 | Manifest in integration/logs | Codex host run | PASS | ../integration/logs/2026-10-08_host-checks.txt | Host/fake-register evidence only |
| 2026-10-08 | S3-U02 | Not yet run on Pico | Pending operator | PENDING | none | Test firmware built only |
| 2026-10-08 | S3-U04/U05/U06/U07/U08 | ../integration/logs/2026-10-08_s3-proportional-source-sha256.json | Codex host run | PASS | [host log](../integration/logs/2026-10-08_s3-proportional-host-checks.txt) | P/capture/recovery and retained ADC/local/health checks; no physical result |
| 2026-10-08 | S3-U06 | Same source manifest | Pending Pico operator | PENDING | [build evidence](../integration/logs/2026-10-08_s3-proportional-evidence.json) | TEST self-check compiled only; host PASS above is separate |
| 2026-10-08 | S3-U09 | Future implementation | Pending | PENDING | none | Barcode deferred; no decode run |

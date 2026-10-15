# Subsystem4 tests

Fresh definitions/results after the authorized reset. Previous results are recoverable only in Git and are not current PASS evidence. Firmware fixtures execute only in inhibited TEST; no host fake-kernel runner is used.

## Firmware cases

Each row below requires every grouped assertion to succeed. Expected PASS is a requirement, not an executed result. Fixture source: [subsystem4_test.c](subsystem4_test.c). Synthetic values are not hardware calibration.

| ID | Required behavior | Method |
| --- | --- | --- |
| S4-U03 | invalid window preserves accepted peak | Inhibited firmware fixture |
| S4-U04 | new-run snapshot clears before WAIT/error poll | Inhibited firmware fixture |
| S4-U05 | 100 fresh samples; repeated acquisition cannot count twice | Inhibited firmware fixture |
| S4-U06 | movement, stale acquisition, exact deadline and cancellation | Inhibited firmware fixture |
| S4-U07 | pivot telemetry independent of straight-travel hump filter | Inhibited firmware fixture |
| S4-U08 | valid climb/crest/descent/baseline then invalid-after-valid | Inhibited firmware fixture |
| S4-U09 | acquisition failure invalidates calibration/partial window and retains accepted last/max | Inhibited firmware fixture |
| S4-U10 | hardware-context reinitialization preserves stop-only results and clears a new run | Inhibited firmware fixture |

## Results

| Date | IDs | Revision / image SHA-256 | Profile/configuration | Tester | Result | Measurements / evidence |
| --- | --- | --- | --- | --- | --- | --- |

Firmware execution: **PENDING**. Append actual complete console evidence only after execution. Builds/compiler checks belong in the integration log and do not fill this table with fixture PASS results.

Physical coverage and procedures: [system cases](../system/TEST_LOG.md), [GUIDE](../../GUIDE.md), [testing policy](../README.md). Include source, image, power/wiring, calibration, mapping request/revision, reference/observed measurements, time and evidence for every real run.

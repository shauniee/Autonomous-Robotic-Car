# Subsystem5 tests

Fresh definitions/results after the authorized reset. Previous results are recoverable only in Git and are not current PASS evidence. Firmware fixtures execute only in inhibited TEST; no host fake-kernel runner is used.

## Firmware cases

Each row below requires every grouped assertion to succeed. Expected PASS is a requirement, not an executed result. Fixture source: [subsystem5_test.c](subsystem5_test.c). Synthetic values are not hardware calibration.

| ID | Required behavior | Method |
| --- | --- | --- |
| S5-U04 | valid profile nearest point; missing Echo cannot grant clearance | Inhibited firmware fixture |
| S5-U05 | cancel dominates timeout/movement including center restore | Inhibited firmware fixture |
| S5-U06 | profile geometry includes measured sensor mounting offset | Inhibited firmware fixture |

## Results

| Date | IDs | Revision / image SHA-256 | Profile/configuration | Tester | Result | Measurements / evidence |
| --- | --- | --- | --- | --- | --- | --- |

Firmware execution: **PENDING**. Append actual complete console evidence only after execution. Builds/compiler checks belong in the integration log and do not fill this table with fixture PASS results.

Physical coverage and procedures: [system cases](../system/TEST_LOG.md), [GUIDE](../../GUIDE.md), [testing policy](../README.md). Include source, image, power/wiring, calibration, mapping request/revision, reference/observed measurements, time and evidence for every real run.

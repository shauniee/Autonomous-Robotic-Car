# Subsystem3 tests

Fresh definitions/results after the authorized reset. Previous results are recoverable only in Git and are not current PASS evidence. Firmware fixtures execute only in inhibited TEST; no host fake-kernel runner is used.

## Firmware cases

Each row below requires every grouped assertion to succeed. Expected PASS is a requirement, not an executed result. Fixture source: [subsystem3_test.c](subsystem3_test.c). Synthetic values are not hardware calibration.

| ID | Required behavior | Method |
| --- | --- | --- |
| S3-U06 | P guidance sign, bound and stale inhibition | Inhibited firmware fixture |
| S3-U12 | age across wrap never refreshes acquisition | Inhibited firmware fixture |
| S3-U09 | printed A/Z forward/reverse and invalid element | Inhibited firmware fixture |
| S3-U14 | one beep only for joint completion; stop/failure/deadline dominates | Inhibited firmware fixture |
| S3-U15 | A-Z both scales/orientations and zero-width rejection | Inhibited firmware fixture |
| S3-U16 | real capture engine, count wrap, single queue event and retained report | Inhibited firmware fixture |
| S3-U17 | busy white press ignored, fresh later press accepted, GP21 cancels | Inhibited firmware fixture |

## Results

| Date | IDs | Revision / image SHA-256 | Profile/configuration | Tester | Result | Measurements / evidence |
| --- | --- | --- | --- | --- | --- | --- |

Firmware execution: **PENDING**. Append actual complete console evidence only after execution. Builds/compiler checks belong in the integration log and do not fill this table with fixture PASS results.

Physical coverage and procedures: [system cases](../system/TEST_LOG.md), [GUIDE](../../GUIDE.md), [testing policy](../README.md). Include source, image, power/wiring, calibration, mapping request/revision, reference/observed measurements, time and evidence for every real run.

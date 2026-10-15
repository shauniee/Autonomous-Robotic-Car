# Subsystem1 tests

Fresh definitions/results after the authorized reset. Previous results are recoverable only in Git and are not current PASS evidence. Firmware fixtures execute only in inhibited TEST; no host fake-kernel runner is used.

## Firmware cases

Each row below requires every grouped assertion to succeed. Expected PASS is a requirement, not an executed result. Fixture source: [subsystem1_test.c](subsystem1_test.c). Synthetic values are not hardware calibration.

| ID | Required behavior | Method |
| --- | --- | --- |
| S1-U04 | bounded telemetry JSON and original age; no radio test | Inhibited firmware fixture |
| S1-U06 | fragmented status only; malformed and movement requests rejected | Inhibited firmware fixture |
| S1-U07 | bounded heartbeat/status JSON; exact capacity and max counters | Inhibited firmware fixture |
| S1-U09 | 26-entry fragmented mapping and bounded rejection | Inhibited firmware fixture |
| S1-U10 | ack failure/reconnect retry, stale callback, bounded backpressure | Inhibited firmware fixture |
| S1-U11 | every topic rejects wildcards and duplicates | Inhibited firmware fixture |
| S1-U12 | robot summary and bounded event history/cursor | Inhibited firmware fixture |
| S1-U13 | bounded read-only event retrieval request | Inhibited firmware fixture |

## Results

| Date | IDs | Revision / image SHA-256 | Profile/configuration | Tester | Result | Measurements / evidence |
| --- | --- | --- | --- | --- | --- | --- |

Firmware execution: **PENDING**. Append actual complete console evidence only after execution. Builds/compiler checks belong in the integration log and do not fill this table with fixture PASS results.

Physical coverage and procedures: [system cases](../system/TEST_LOG.md), [GUIDE](../../GUIDE.md), [testing policy](../README.md). Include source, image, power/wiring, calibration, mapping request/revision, reference/observed measurements, time and evidence for every real run.

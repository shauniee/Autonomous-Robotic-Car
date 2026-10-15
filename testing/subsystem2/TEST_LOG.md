# Subsystem2 tests

Fresh definitions/results after the authorized reset. Previous results are recoverable only in Git and are not current PASS evidence. Firmware fixtures execute only in inhibited TEST; no host fake-kernel runner is used.

## Firmware cases

Each row below requires every grouped assertion to succeed. Expected PASS is a requirement, not an executed result. Fixture source: [subsystem2_test.c](subsystem2_test.c). Synthetic values are not hardware calibration.

| ID | Required behavior | Method |
| --- | --- | --- |
| S2-U06 | quadrature direction and illegal transition | Inhibited firmware fixture |
| S2-U07 | renewal preserves origin/deadline; completion requires both wheels | Inhibited firmware fixture |
| S2-U08 | old run command cannot replay after reset | Inhibited firmware fixture |
| S2-U09 | bounded local commissioning requires explicit electrical release | Inhibited firmware fixture |
| S2-U10 | exact expiry brakes; no one-wheel premature completion | Inhibited firmware fixture |
| S2-U11 | lease wrap and deadline boundary | Inhibited firmware fixture |
| S2-U12 | left/right/U-turn use signed encoder travel | Inhibited firmware fixture |
| S2-U13 | signed wheel PID respects independent output bounds | Inhibited firmware fixture |

## Results

| Date | IDs | Revision / image SHA-256 | Profile/configuration | Tester | Result | Measurements / evidence |
| --- | --- | --- | --- | --- | --- | --- |

Firmware execution: **PENDING**. Append actual complete console evidence only after execution. Builds/compiler checks belong in the integration log and do not fill this table with fixture PASS results.

Physical coverage and procedures: [system cases](../system/TEST_LOG.md), [GUIDE](../../GUIDE.md), [testing policy](../README.md). Include source, image, power/wiring, calibration, mapping request/revision, reference/observed measurements, time and evidence for every real run.

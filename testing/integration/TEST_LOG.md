# Integration tests

Fresh definitions/results after the authorized reset. Previous results are recoverable only in Git and are not current PASS evidence. Firmware fixtures execute only in inhibited TEST; no host fake-kernel runner is used.

## Firmware cases

Each row below requires every grouped assertion to succeed. Expected PASS is a requirement, not an executed result. Fixture source: [mission_test.c](mission_test.c). Synthetic values are not hardware calibration.

| ID | Required behavior | Method |
| --- | --- | --- |
| INT-15 | valid complete settings accepted; missing headroom/course rejected | Inhibited firmware fixture |
| INT-16 | positive junction/finish and travel/action/dwell boundaries | Inhibited firmware fixture |
| INT-22 | distinct consecutive acquisitions, cache and wrap | Inhibited firmware fixture |
| INT-23 | braking begins outside center-based pivot envelope | Inhibited firmware fixture |
| INT-21 | any four letters, one per motion; 22 remain unmapped | Inhibited firmware fixture |

## Build and integrated cases

| ID | Required behavior | Method |
| --- | --- | --- |
| INT-17 | Four profiles, normal fixture exclusion, strict changed-code checks, source/image identity, unchanged main/video | Target compiler/build and repository audit |
| INT-18 | Atomic four-letter stopped replacement with live provider revision; busy/invalid/backpressure rejection and unchanged-image reboot/reload | Actual Pico and broker; PENDING |
| INT-19 | Combined calibration, leased actions, capture rearm, scan cancellation and original-route recovery | Actual Pico/controller/feature trials; PENDING |
| INT-20 | Known source defects corrected; regression definitions present; measured/course limitations explicitly retained | Source review and traceability; no physical proof |

## Results

| Date | IDs | Revision / image SHA-256 | Profile/configuration | Tester | Result | Measurements / evidence |
| --- | --- | --- | --- | --- | --- | --- |

Firmware execution: **PENDING**. Append actual complete console evidence only after execution. Builds/compiler checks belong in the integration log and do not fill this table with fixture PASS results.

Physical coverage and procedures: [system cases](../system/TEST_LOG.md), [GUIDE](../../GUIDE.md), [testing policy](../README.md). Include source, image, power/wiring, calibration, mapping request/revision, reference/observed measurements, time and evidence for every real run.

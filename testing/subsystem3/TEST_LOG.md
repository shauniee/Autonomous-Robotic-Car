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

| S3-U18 | Black/white capture, opposite polarity, stale guidance and low-contrast reset | Inhibited firmware fixture |
| S3-U19 | IR calibration admission, wrapped capture timeout, duplicate/invalid sample handling and opposite polarity | Inhibited firmware fixture |
| S3-U20 | Service mapping enum/revision lifecycle and mission mapping's exactly-four-distinct-action validation (missing, duplicate and invalid values rejected) | Inhibited firmware fixture |
| S3-U21 | Mission sensor freshness, stopped mapping admission, broad-black dwell continuity, confirmed junction/finish thresholds and geometry guards | Inhibited firmware fixture |
| S3-U22 | IR normalization endpoints/midpoint, exact white-enter/white-exit threshold behavior and strict hysteresis, invalid raw rejection and zero guidance | Inhibited firmware fixture |
| S3-U23 | Code 39 valid decode, ambiguous width rejection, malformed frame rejection and invalid configuration rejection | Inhibited firmware fixture |
| S3-U24 | Unmapped symbol remains a recorded observation and is excluded from the action event queue | Inhibited firmware fixture |
| S3-U25 | Encoder epoch change rejects capture input; bounded frame deadline returns service to quiet admission | Inhibited firmware fixture |

## Results

| Date | IDs | Revision / image SHA-256 | Profile/configuration | Tester | Result | Measurements / evidence |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-15 | S3-U06, S3-U09, S3-U12, S3-U14, S3-U15, S3-U16, S3-U17, S3-U18 | `61bc484`; UF2 `22403565d19f666e49cc46fead2dd57c9fc8fb5a0f55b438497421164920f254` | USB-only, wireless IR-only TEST; missing IMU/sonar; motors inhibited | Nicholas / Codex capture | Firmware PASS | [Complete 45-check capture](../integration/logs/2026-10-15_134410_950691_pico-console.txt), [summary](../integration/logs/2026-10-15_134410_950691_pico-console.json); logic/timer/kernel checks only, no powered acceptance |

Actual inhibited firmware execution is recorded above. Physical acceptance remains pending. Earlier compiled-only evidence retains its original status; builds never count as executed fixtures.

Physical coverage and procedures: [system cases](../system/TEST_LOG.md), [GUIDE](../../GUIDE.md), [testing policy](../README.md). Include source, image, power/wiring, calibration, mapping request/revision, reference/observed measurements, time and evidence for every real run.

Latest executed image: `33ea475`, UF2 `f554c1f1280098cfc6c3278992c0d5877f744dab0f07a1ef84d02e6b674318b0`; USB-only IR-only TEST, motors inhibited, tester Nicholas/Codex. **Firmware PASS** for S3-U06, S3-U09, S3-U12, S3-U14, S3-U15, S3-U16, S3-U17, S3-U18; [complete 45-check capture](../integration/logs/2026-10-15_152409_309541_pico-console.txt), [summary](../integration/logs/2026-10-15_152409_309541_pico-console.json). Physical acceptance remains pending.


Exact target fixture host run at `1bd0ffd`: PASS, 15 checks (S3-U06, S3-U09, S3-U12, S3-U14–S3-U25); full output in [host fixture log](../pure/logs/2026-10-15_fixture-suite.txt). The runner compiles the existing `subsystemN_test.c` unchanged and real production algorithm sources with the host compiler; only the test report output is adapted. It does not provide T-Kernel, GPIO, timer, firmware or physical-hardware evidence.

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
| 2026-10-15 | S4-U03, S4-U04, S4-U05, S4-U06, S4-U07, S4-U08, S4-U09, S4-U10 | `61bc484`; UF2 `22403565d19f666e49cc46fead2dd57c9fc8fb5a0f55b438497421164920f254` | USB-only, wireless IR-only TEST; missing IMU/sonar; motors inhibited | Nicholas / Codex capture | Firmware PASS | [Complete 45-check capture](../integration/logs/2026-10-15_134410_950691_pico-console.txt), [summary](../integration/logs/2026-10-15_134410_950691_pico-console.json); logic/timer/kernel checks only, no powered acceptance |

Actual inhibited firmware execution is recorded above. Physical acceptance remains pending. Earlier compiled-only evidence retains its original status; builds never count as executed fixtures.

Physical coverage and procedures: [system cases](../system/TEST_LOG.md), [GUIDE](../../GUIDE.md), [testing policy](../README.md). Include source, image, power/wiring, calibration, mapping request/revision, reference/observed measurements, time and evidence for every real run.

Latest executed image: `33ea475`, UF2 `f554c1f1280098cfc6c3278992c0d5877f744dab0f07a1ef84d02e6b674318b0`; USB-only IR-only TEST, motors inhibited, tester Nicholas/Codex. **Firmware PASS** for S4-U03, S4-U04, S4-U05, S4-U06, S4-U07, S4-U08, S4-U09, S4-U10; [complete 45-check capture](../integration/logs/2026-10-15_152409_309541_pico-console.txt), [summary](../integration/logs/2026-10-15_152409_309541_pico-console.json). Physical acceptance remains pending.

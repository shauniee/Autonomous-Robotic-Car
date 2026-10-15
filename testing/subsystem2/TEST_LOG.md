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
| S2-U14 | non-finite action origin/feedback and missing command configuration reject safely | Inhibited firmware fixture |

| S2-U15 | freshness exact-age boundary and future/stale sample rejection | Inhibited firmware fixture |
| S2-U16 | same-action lease renewal preserves origin/deadline; identity mutation and replay reject | Inhibited firmware fixture |
| S2-U17 | per-wheel stall fails with zero output and completion acknowledges once | Inhibited firmware fixture |
| S2-U18 | electrical-release bench pulse remains bounded and brakes at exact deadline | Inhibited firmware fixture |
## Results

| Date | IDs | Revision / image SHA-256 | Profile/configuration | Tester | Result | Measurements / evidence |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-15 | S2-U06, S2-U07, S2-U08, S2-U09, S2-U10, S2-U11, S2-U12, S2-U13, S2-U14 | `61bc484`; UF2 `22403565d19f666e49cc46fead2dd57c9fc8fb5a0f55b438497421164920f254` | USB-only, wireless IR-only TEST; missing IMU/sonar; motors inhibited | Nicholas / Codex capture | Firmware PASS | [Complete 45-check capture](../integration/logs/2026-10-15_134410_950691_pico-console.txt), [summary](../integration/logs/2026-10-15_134410_950691_pico-console.json); logic/timer/kernel checks only, no powered acceptance |

Actual inhibited firmware execution is recorded above. Physical acceptance remains pending. Earlier compiled-only evidence retains its original status; builds never count as executed fixtures.

Physical coverage and procedures: [system cases](../system/TEST_LOG.md), [GUIDE](../../GUIDE.md), [testing policy](../README.md). Include source, image, power/wiring, calibration, mapping request/revision, reference/observed measurements, time and evidence for every real run.

Latest executed image: `33ea475`, UF2 `f554c1f1280098cfc6c3278992c0d5877f744dab0f07a1ef84d02e6b674318b0`; USB-only IR-only TEST, motors inhibited, tester Nicholas/Codex. **Firmware PASS** for S2-U06, S2-U07, S2-U08, S2-U09, S2-U10, S2-U11, S2-U12, S2-U13, S2-U14; [complete 45-check capture](../integration/logs/2026-10-15_152409_309541_pico-console.txt), [summary](../integration/logs/2026-10-15_152409_309541_pico-console.json). Physical acceptance remains pending.

S2-U15–S2-U18 are new definitions and have not run on the Pico. USB-only firmware execution is pending. T02/T05/T14 own physical stop/reset, encoder lifecycle, renewal/completion and commissioning acceptance; those hardware gates remain pending. No new physical or fixture PASS is inferred from compilation.




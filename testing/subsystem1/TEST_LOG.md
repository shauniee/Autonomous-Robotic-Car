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
| S1-U12 | robot summary, capacity rejection, bounded history/cursor/reset/saturation | Inhibited firmware fixture |
| S1-U13 | bounded read-only event retrieval request | Inhibited firmware fixture |

## Results

| Date | IDs | Revision / image SHA-256 | Profile/configuration | Tester | Result | Measurements / evidence |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-15 | S1-U04, S1-U06, S1-U07, S1-U09, S1-U10, S1-U11, S1-U12, S1-U13 | `61bc484`; UF2 `22403565d19f666e49cc46fead2dd57c9fc8fb5a0f55b438497421164920f254` | USB-only, wireless IR-only TEST; missing IMU/sonar; motors inhibited | Nicholas / Codex capture | Firmware PASS | [Complete 45-check capture](../integration/logs/2026-10-15_134410_950691_pico-console.txt), [summary](../integration/logs/2026-10-15_134410_950691_pico-console.json); logic/timer/kernel checks only, no powered acceptance |

Actual inhibited firmware execution is recorded above. Physical acceptance remains pending. Earlier compiled-only evidence retains its original status; builds never count as executed fixtures.

Physical coverage and procedures: [system cases](../system/TEST_LOG.md), [GUIDE](../../GUIDE.md), [testing policy](../README.md). Include source, image, power/wiring, calibration, mapping request/revision, reference/observed measurements, time and evidence for every real run.

## Dashboard checks

These laptop web checks are separate from inhibited firmware fixtures. Run `python3 testing/subsystem1/dashboard_check.py --broker 127.0.0.1 --output testing/subsystem1/logs/<new-name>.json` with a real broker. The checker uses a unique `robot/dashboard-check/` root, synthetic [sample data](data/dashboard-sample.json), a temporary loopback HTTP port and no kernel mocks. It never publishes robot commands or changes live `robot/s3` topics. Existing evidence filenames are refused. Optional `--preview-seconds 120` keeps its clearly labelled synthetic feed available for browser inspection.

| ID | Required result |
| --- | --- |
| S1-W01 | Website/assets/API served; repository/credential paths unavailable |
| S1-W02 | Retained presence and telemetry never qualify live robot data |
| S1-W03 | Real broker delivery reaches HTTP readings/history with correct numeric values |
| S1-W04 | Malformed/oversized/non-finite messages preserve the last accepted snapshot |
| S1-W05 | Missing data/heartbeat becomes stale, offline presence wins and accepted peak remains |
| S1-W06 | Broker reconnect requires fresh telemetry and heartbeat |
| S1-W07 | Browser renders known values/units/enums/encounters; desktop/mobile layout works; no script errors |

| Date | IDs | Source / method | Tester | Result | Evidence |
| --- | --- | --- | --- | --- | --- |
| 2026-10-15 | S1-W01-W06 | Source hashes in log; actual local broker + HTTP, synthetic isolated messages | Codex | PASS | [Initial run](logs/2026-10-15_dashboard_broker.json); [preview run](logs/2026-10-15_dashboard_broker_final.json); [committed-source verification](logs/2026-10-15_dashboard_broker_verified.json) |
| 2026-10-15 | S1-W07 | Source hashes / observed browser behavior; synthetic feed separated from configured robot topic | Codex | PASS | [Browser inspection](logs/2026-10-15_dashboard_browser.json) |

Live configured-topic observations belong in the browser record, with the actual state/validity and no invented calibration results. Synthetic sample values are not measured robot outcomes. T12/T13/T15 and complete mission acceptance remain separate physical/integrated gates.

Actual MQTT observations on `61bc484`, USB-only inhibited: [two accepted tables](logs/2026-10-15_available-timer_mapping.json), [three rejected requests](logs/2026-10-15_available-timer_mapping-rejection.json), [advancing telemetry with cache-age issue](logs/2026-10-15_available-timer_mqtt-live.json). INT-18/T12 are partial; these do not complete reboot/recovery/powered acceptance.

Latest executed image: `33ea475`, UF2 `f554c1f1280098cfc6c3278992c0d5877f744dab0f07a1ef84d02e6b674318b0`; USB-only IR-only TEST, motors inhibited, tester Nicholas/Codex. **Firmware PASS** for S1-U04, S1-U06, S1-U07, S1-U09, S1-U10, S1-U11, S1-U12, S1-U13; [complete 45-check capture](../integration/logs/2026-10-15_152409_309541_pico-console.txt), [summary](../integration/logs/2026-10-15_152409_309541_pico-console.json). Physical acceptance remains pending.

`33ea475` actual MQTT follow-up: [12 live samples](logs/2026-10-15_available-release_mqtt-live.json) PASS, IR ages 20–40 ms and Motion ages 10–30 ms, raw encoders valid and motors inhibited. [Two mapping changes](logs/2026-10-15_available-release_mapping.json) accepted revisions 1/2 without reflashing. [Passive retained-topic check](logs/2026-10-15_available-release_retained-mapping.json) found no retained mapping; broker permits retention, so this is an observation rather than an enforcement guarantee.

INT-18 reboot/reload subset PASS at `33ea475`: [normal reboot and reload](logs/2026-10-15_available-release_reboot-reload.json) observed fresh revision 0 before reload and matching revision 1 acknowledgement afterward, with no recompile/reflash and motors inhibited. The [earlier BOOTSEL attempt](logs/2026-10-15_available-release_reboot-attempt.json) is retained as incomplete. Full INT-18 busy/backpressure/moving transport checks and T12/T13 remain partial.

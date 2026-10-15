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

Firmware execution: **PENDING**. Append actual complete console evidence only after execution. Builds/compiler checks belong in the integration log and do not fill this table with fixture PASS results.

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

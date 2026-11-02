# S1 test log

## Contents

- [1. Test cases](#1-test-cases)
- [2. Results](#2-results)
- [Binary protocol replacement cases — 2026-10-30](#binary-protocol-replacement-cases--2026-10-30)
- [Documentation alignment — 2026-10-29](#documentation-alignment--2026-10-29)

Owner: Shaun / communications. Current S1 runs bounded reporting in I/O, uses the cross-core event mutex and kernel TX/RX bridge; it has no application task. Retired template IDs: S1-U01 cyclic handler, S1-U02 sleeping LED task, S1-U03 GP19 LED at 2 Hz. No template result was recorded. The historical restoration below used heartbeat/status JSON; current binary cases are defined in the replacement section. No Pico hardware result is claimed.

## 1. Test cases

| ID | Scope | Method | Expected |
| --- | --- | --- | --- |
| S1-U04 | Bounded JSON | Host actual encoder; TEST image contains a smaller self-check | Null unavailable values, negative/positive fixed decimals, original timestamp/age including wrap; exact capacity succeeds, undersized/oversized or unterminated input fails without overrun |
| S1-U05 | Latest snapshot and adapter | Host actual S1 store/S3 adapter with fake kernel, plus simultaneous pthread producer/consumer using try-lock | Safe before init and on allocation/busy failure; TMO_POL only; complete paired values, sequence advances on stored update, independent producer storage; requested duties and TEST flag reported without controller changes |

| S1-U06 | Status request parser | Firmware TEST self-check, pending execution | Accept exact status-only JSON with whitespace and fragments; reject malformed, duplicate/unknown fields, movement commands, overflow, short final fragment and embedded NUL; reset discards fragments |
| S1-U07 | Network status encoder | Firmware TEST self-check, pending execution | Valid bounded JSON at max uint32 counters; exact capacity succeeds, too-small capacity clears output; no snapshot is explicit |
| S1-U08 | Restored laptop helper | Isolated localhost Mosquitto with synthetic messages; monitor/status CLI | Subscribe before request; exact nonretained status-only request, live response, cached retained status alone times out, telemetry/heartbeat display, missing-heartbeat notice, no start/stop action |

## 2. Results

Historical migration status: no S1 project results were recorded while this subsystem was an inactive placeholder.

| Date | ID | Source | Tester | Result | Evidence | Limits |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-08 | S1-U04/U05 | [source manifest](../integration/logs/2026-10-08_s3-mqtt-source-sha256.json) | Codex host run | PASS | [host log](../integration/logs/2026-10-08_s3-mqtt-host-checks.txt) | Real modules with fake mutex/clock; not Pico scheduling or WiFi |
| 2026-10-08 | S1-U04 | Wireless TEST image | On-target run pending | PENDING | No Pico console log | Compiled self-check is not an executed on-target result |
| 2026-10-09 | S1-U08 | [restoration manifest](../integration/logs/2026-10-09_s1-restore_evidence.json) | Codex laptop run | PASS | [laptop check](../integration/logs/2026-10-09_s1-restore_laptop-check.txt) | Actual helper/localhost broker, synthetic fixture; not Pico transport |
| 2026-10-09 | S1-U04/U06/U07 | Same manifest, wireless TEST UF2 checksum | On-target run pending | PENDING | [TEST build](../integration/logs/2026-10-09_s1-restore_wireless-test-build.txt) | Compiled self-checks, not executed on Pico |

## Documentation alignment — 2026-10-29

Planning note only; no test was executed and no historical result is changed.
The [central plan](../../plan.md) owns future implementation sequence;
[mission-readiness](../mission-readiness.md) owns target physical procedures.
Earlier task splits, GP20 reset flows, JSON messages, watchdog assumptions and
configured timing targets describe their recorded revisions. Do not interpret
them as target passes. Use the applicable procedure revision, exact source and
new evidence filenames for reruns; never overwrite evidence or reuse retired IDs.

## Binary protocol replacement cases — 2026-10-30

Procedure revision only: retire S1-U04 through S1-U08 for current acceptance.
Their JSON/fake-kernel/localhost definitions and results above remain historical;
none is reused. The firmware codec self-check now prints S1-U09. Its compiled
presence does not establish an on-target pass. Current test definitions:

| ID | Scope / method | Expected |
| --- | --- | --- |
| S1-U09 | Binary schema-v2 codec; `test_mqtt_frame.c`, firmware startup subset | Golden bytes, exact 23/11/5/3/2 lengths, short/oversized buffers, version/type/reserved/range rejection; snapshot reply shares telemetry body |
| S1-U10 | Reporting/events/mapping; `test_s1_report.c`, `test_mission_mapping.c` | Packed events round-trip; bounded overflow/drop saturation; backpressure retains oldest; stopped install precedes ACK; busy/invalid mapping rejected; 100000 concurrent request/reply exchanges |
| S1-U11 | Kernel bridge stress; `test_s1_stress.c` | 300000 frames without corruption/reordering; 5000000 word reads without tearing; no networking or motor authority added |
| S1-U12 | Binary laptop helper; `scripts/mqtt_client.py --self-test` | Same schema-v2 vectors and seven-state labels; no claim of broker/Pico transport from self-test |
| S1-U13 | Six owner fault bytes; `test_mqtt_frame.c`, `test_robot_data.c`, `test_safety.c` | All 48 walking bits round-trip in bytes 4..9; per-owner start/stop masks tested independently; report-only bits remain report-only |
| S1-M01 | Pending physical binary subscription; T12/T13 | Current Pico-originated telemetry and mapping ACK, all six fault bytes under controlled fault injection, reconnect with no stale replay; record flashed image and raw subscriber capture |

Missing sonar echo is **not** a universal stop cause: the
[central fault policy](../../docs/architecture.md#fault-bitmask) permits ordinary
tracking with a report-only no-echo bit. Bypass offset requires supported front
range and brakes on unknown range. INT-24 tests both software contexts. T08/T10
and S1-M01 must capture the actual fault byte, motor response and timing on the
robot; these physical observations are still pending.

| Date | IDs | Source | Tester | Result | Evidence / limits |
| --- | --- | --- | --- | --- | --- |
| 2026-10-30 | S1-U09..U13 | Base `62ee5226e16a3c501d94998291944753c9662dfc` plus [working-source manifest](../integration/logs/2026-10-30_INT-24-accepted-manifest.json) | Codex | PASS (host only) | [Full suite](../integration/logs/2026-10-30_INT-24-accepted-host.txt); codec, reporting, mapping, bridge, helper and fault masks. Firmware startup subset compiled in both TEST profiles; execution pending |
| 2026-10-30 | S1-M01 | Current-image trial to record | Operator pending | PENDING | No Pico subscription, controlled physical fault injection or timing capture executed |

2026-10-30 procedure follow-up, INT-25: remove the unused synchronous
`s1_handle_command` tests. S1-U10 now exercises event reporting in
`test_s1_report.c` and the actual asynchronous hand-off separately in
`test_mission_mapping.c`; this does not execute kernel-bound `s1_step`.
[Host suite](../integration/logs/2026-10-30_INT-25-accepted-host.txt) passes at the
[INT-25 working-source manifest](../integration/logs/2026-10-30_INT-25-accepted-manifest.json).
All four profiles compile. Insertion uses the existing 2 ms mutex timeout;
failed acquisitions increment single-writer per-core saturating diagnostic
words (`lock_misses[0]` Terrain/I/O, `[1]` Mission). Ring overwrite drops remain
separate. No new MQTT counters or physical delivery claim is added.
Actual cross-core contention, forced timeout and UINT32_MAX saturation require
controlled on-target injection; those checks have not been executed.

# S1 test log

## Contents

- [1. Test cases](#1-test-cases)
- [2. Results](#2-results)
- [Documentation alignment — 2026-10-29](#documentation-alignment--2026-10-29)

Owner: Shaun / communications. S1 now owns the optional latest S3 snapshot and bounded encoder, with one mutex and no task. Retired template IDs: S1-U01 cyclic handler, S1-U02 sleeping LED task, S1-U03 GP19 LED at 2 Hz. No template result was recorded. Heartbeat/status reporting and a status-only subscription are restored from the earlier S1 branch in the existing kernel service. No Pico hardware result is claimed.

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

# S1 test log

Owner: Shaun / communications. S1 owns the optional latest snapshot and bounded encoder, with one mutex and no task. Its publication-time adapter reads the public S3 IR, coordinator and S2 output snapshots without waiting. Retired template IDs: S1-U01 cyclic handler, S1-U02 sleeping LED task, S1-U03 GP19 LED at 2 Hz. No template result was recorded. Heartbeat/status reporting and a status-only subscription run in the existing kernel service. The localhost dashboard displays observations only. No Pico hardware result is claimed here.

## 1. Test cases

| ID | Scope | Method | Expected |
| --- | --- | --- | --- |
| S1-U04 | Bounded JSON | Host actual encoder; TEST image contains a smaller self-check | Null unavailable values, negative/positive fixed decimals, original timestamp/age including wrap; exact capacity succeeds, undersized/oversized or unterminated input fails without overrun |
| S1-U05 | Latest snapshot and adapter | Host actual S1 store/S3 adapter with fake kernel, plus simultaneous pthread producer/consumer using try-lock | Safe before init and on allocation/busy failure; TMO_POL only; complete paired values, sequence advances on stored update, independent producer storage; requested duties and TEST flag reported without controller changes |

| S1-U06 | Status request parser | Firmware TEST self-check, pending execution | Accept exact status-only JSON with whitespace and fragments; reject malformed, duplicate/unknown fields, movement commands, overflow, short final fragment and embedded NUL; reset discards fragments |
| S1-U07 | Network status encoder | Firmware TEST self-check, pending execution | Valid bounded JSON at max uint32 counters; exact capacity succeeds, too-small capacity clears output; no snapshot is explicit |
| S1-U08 | Restored laptop helper | Isolated localhost Mosquitto with synthetic messages; monitor/status CLI | Subscribe before request; exact nonretained status-only request, live response, cached retained status alone times out, telemetry/heartbeat display, missing-heartbeat notice, no start/stop action |
| S1-U09 | S3/coordinator/S2 telemetry mapping | Firmware TEST fixtures; on-target execution pending | Original IR time/sequence and coordinator observation time, white/black -1/+1 maps to schema-1 fractions 0/1, requested versus applied PWM distinguished; absent Motion is unavailable; coordinator age >60ms rejected with wrap-safe elapsed time; invalid finite/range/required-source inputs rejected |
| S1-U10 | Additive schema and bounded Motion diagnostics | Firmware TEST fixtures; on-target execution pending | Max counters/labels fit 768 bytes; exact capacity succeeds, undersized clears output; unavailable Motion is null; sensor/output age wraps correctly |
| S1-U11 | HTML/SSE observations and browser view | Actual dashboard/Paho/local broker, separate synthetic topic; manual Chrome inspection | Fixed assets, latest SSE, original schema-1 compatibility, null/invalid/stale values, TEST inhibition label, no publish controls; 390px view has no horizontal overflow or browser errors |
| S1-U12 | Dashboard broker recovery | Actual dashboard with temporary localhost broker on port 18884 | HTTP starts before broker, bounded initial retry, disconnect visible, automatic reconnect/resubscription; existing port-1883 broker unchanged |

## 2. Results

Historical migration status: no S1 project results were recorded while this subsystem was an inactive placeholder.

| Date | ID | Source | Tester | Result | Evidence | Limits |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-08 | S1-U04/U05 | [source manifest](../integration/logs/2026-10-08_s3-mqtt-source-sha256.json) | Codex host run | PASS | [host log](../integration/logs/2026-10-08_s3-mqtt-host-checks.txt) | Real modules with fake mutex/clock; not Pico scheduling or WiFi |
| 2026-10-08 | S1-U04 | Wireless TEST image | On-target run pending | PENDING | No Pico console log | Compiled self-check is not an executed on-target result |
| 2026-10-09 | S1-U08 | [restoration manifest](../integration/logs/2026-10-09_s1-restore_evidence.json) | Codex laptop run | PASS | [laptop check](../integration/logs/2026-10-09_s1-restore_laptop-check.txt) | Actual helper/localhost broker, synthetic fixture; not Pico transport |
| 2026-10-09 | S1-U04/U06/U07 | Same manifest, wireless TEST UF2 checksum | On-target run pending | PENDING | [TEST build](../integration/logs/2026-10-09_s1-restore_wireless-test-build.txt) | Compiled self-checks, not executed on Pico |
| 2026-10-09 | S1-U09/U10 | [dashboard source manifest](logs/2026-10-09_dashboard-source.json) | On-target run pending | PENDING | Added firmware fixtures | Integrated dependency headers/link/build and actual Pico execution belong to the integrated run; no pass inferred |
| 2026-10-09 | S1-U11 | Same manifest; Python 3.9.6 / Paho 2.1.0 | Codex laptop/browser checks | PASS | [dashboard observations](logs/2026-10-09_dashboard-observations.txt) | Separate synthetic MQTT topic; no Pico radio or motor claim; Chrome desktop and 390px visual inspection |
| 2026-10-09 | S1-U12 | Same manifest | Codex isolated laptop check | PASS | [recovery observations](logs/2026-10-09_dashboard-recovery.txt) | Temporary port-18884 broker, not the Pico transport or real-robot network-loss trial |
| 2026-10-09 | S1-U11 supplement | [dashboard hardening manifest](logs/2026-10-09_dashboard-hardening-source.json) | Codex laptop/browser check | PASS | [numeric overflow](logs/2026-10-09_dashboard-overflow.txt) | Exponent overflow rejected before caching; same live SSE receives valid follow-up. Revised history script loaded in Chrome without warnings/errors; 60s eviction/gap break reviewed in source, not a timed hardware test |

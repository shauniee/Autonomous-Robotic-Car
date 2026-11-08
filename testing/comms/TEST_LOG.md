# Comms test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)

## Test cases

| ID | Scope | Method | Expected |
| --- | --- | --- | --- |
| S1-U04 | Bounded JSON | Host actual encoder; TEST image contains a smaller self-check | Null unavailable values, negative/positive fixed decimals, original timestamp/age including wrap; exact capacity succeeds, undersized/oversized or unterminated input fails without overrun |
| S1-U05 | Latest snapshot and adapter | Host actual Comms store/line_barcode adapter with fake kernel, plus simultaneous pthread producer/consumer using try-lock | Safe before init and on allocation/busy failure; TMO_POL only; complete paired values, sequence advances on stored update, independent producer storage; requested duties and TEST flag reported without controller changes |
| S1-U06 | Status request parser | Firmware TEST self-check, pending execution | Accept exact status-only JSON with whitespace and fragments; reject malformed, duplicate/unknown fields, movement commands, overflow, short final fragment and embedded NUL; reset discards fragments |
| S1-U07 | Network status encoder | Firmware TEST self-check, pending execution | Valid bounded JSON at max uint32 counters; exact capacity succeeds, too-small capacity clears output; no snapshot is explicit |
| S1-U08 | Restored laptop helper | Isolated localhost Mosquitto with synthetic messages; monitor/status CLI | Subscribe before request; exact nonretained status-only request, live response, cached retained status alone times out, telemetry/heartbeat display, missing-heartbeat notice, no start/stop action |
| S1-U09 | Binary schema-v2 codec | `test_mqtt_frame.c`, firmware startup subset | Golden bytes, exact 23/11/5/3/2 lengths, short/oversized buffers, version/type/reserved/range rejection; snapshot reply shares telemetry body |
| S1-U10 | Reporting/events/mapping | `test_comms_report.c`, `test_mission_mapping.c` | Packed events round-trip; bounded overflow/drop saturation; backpressure retains oldest; stopped install precedes ACK; busy/invalid mapping rejected; 100000 concurrent request/reply exchanges |
| S1-U11 | Kernel bridge stress | `test_comms_stress.c` | 300000 frames without corruption/reordering; 5000000 word reads without tearing; no networking or motor authority added |
| S1-U12 | Binary laptop helper | `scripts/mqtt_client.py --self-test` | Same schema-v2 vectors and seven-state labels; no claim of broker/Pico transport from self-test |
| S1-U13 | Six owner fault bytes (retired by schema v3; see S1-U14) | `test_mqtt_frame.c`, `test_robot_data.c`, `test_safety.c` | All 48 walking bits round-trip in bytes 4..9; per-owner start/stop masks tested independently; report-only bits remain report-only |
| S1-U14 | 12-bit fault word, schema v3 | `test_mqtt_frame.c`, `mqtt_client.py --self-test` | Each defined fault bit travels alone in its own word bit and returns in its owner byte; undefined owner bits are not sent; reserved bits 12-15 are rejected; all 12 together round-trip |
| S1-M01 | Pending physical binary subscription | T12/T13 | Current Pico-originated telemetry and mapping ACK, all six fault bytes under controlled fault injection, reconnect with no stale replay; record flashed image and raw subscriber capture |

## Results

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-08 | S1-U04/U05 | Not recorded | Codex | PASS (host) | — | Real modules with fake mutex/clock; not Pico scheduling or WiFi |
| 2026-10-08 | S1-U04 | Wireless TEST image | — | PENDING (build) | No Pico console log | Compiled self-check is not an executed on-target result |
| 2026-10-09 | S1-U04/U06/U07 | Same manifest, wireless TEST UF2 checksum | — | PENDING (build) | TEST build | Compiled self-checks, not executed on Pico |
| 2026-10-09 | S1-U08 | Not recorded | Codex | PASS (host) | — | Actual helper/localhost broker, synthetic fixture; not Pico transport |
| 2026-10-30 | S1-M01 | Current-image trial to record | — | PENDING | — | No Pico subscription, controlled physical fault injection or timing capture executed |
| 2026-10-30 | S1-U09..U13 | `62ee522`+ | Codex | PASS (host) | — | PASS (host only). Full suite; codec, reporting, mapping, bridge, helper and fault masks. Firmware startup subset compiled in both TEST profiles; execution pending |
| 2026-10-30 | S1-U10 | `62ee522`+ | Codex | PASS (host, build) | At tag | INT-25: removed unused `s1_handle_command` tests; kernel-bound `s1_step` not run |

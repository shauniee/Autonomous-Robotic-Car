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
| S1-U15 | Mac broker and terminal transport | Historical retired host runner, isolated loopback broker and CLI subscriber, actual JSON encoder fixture | Valid bounded JSON arrives; refusal/restart/reconnect; new subscriber gets no retained history; no Pico claim |
| S1-U16 | Restored Comms kernel reporting transport | Wireless TEST Pico run with broker/operator fault injection; pending | Status subscription before Online; 1 s heartbeat, retained online/offline will, authenticated broker; latest line_barcode schema unchanged; malformed/drive/start/stop/calibration requests rejected; status response rate bounded; 15 s attempt deadline and 1..30 s retry; link loss clears fragments, no stale replay or control calls; measure timer/stack budget |

## Results

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-08 | S1-U04/U05 | Not recorded | Codex | PASS (host) | — | Real modules with fake mutex/clock; not Pico scheduling or WiFi |
| 2026-10-08 | S1-U04 | Wireless TEST image | — | PENDING (build) | No Pico console log | Compiled self-check is not an executed on-target result |
| 2026-10-08 | S1-U15 | Initial pre-PTY host harness | Codex | FAIL | — | Timed out waiting for pipe-buffered CLI debug output; fixed harness with terminal-style PTY; robot code unaffected |
| 2026-10-08 | S1-U15 | Same manifest as final host run | Codex | PASS (host) | — | Real Mosquitto loopback, synthetic C-encoded fixture; not firmware/Pico transport |
| 2026-10-09 | S1-U04/U06/U07 | Same manifest, wireless TEST UF2 checksum | — | PENDING (build) | TEST build | Compiled self-checks, not executed on Pico |
| 2026-10-09 | S1-U08 | Not recorded | Codex | PASS (host) | — | Actual helper/localhost broker, synthetic fixture; not Pico transport |
| 2026-10-09 | S1-U16 | Same manifest | — | PENDING (build) | No Pico service log | Build and laptop-fixture checks do not execute kernel transport/retry/subscription on hardware |
| 2026-10-30 | S1-M01 | Current-image trial to record | — | PENDING | — | No Pico subscription, controlled physical fault injection or timing capture executed |
| 2026-10-30 | S1-U09..U13 | `62ee522`+ | Codex | PASS (host) | — | PASS (host only). Full suite; codec, reporting, mapping, bridge, helper and fault masks. Firmware startup subset compiled in both TEST profiles; execution pending |
| 2026-10-30 | S1-U10 | `62ee522`+ | Codex | PASS (host, build) | At tag | INT-25: removed unused `s1_handle_command` tests; kernel-bound `s1_step` not run |
| 2026-10-30 | S1-U17 | `8be851b`+ | Codex | PASS (build) | — | Wi-Fi startup diagnostic build. Motor-inhibited TEST image |
| 2026-10-30 | S1-U18 | `8be851b`+ | Codex | OBSERVED (car) | [S1-U18-console.txt](logs/2026-10-30_S1-U18-console.txt) | Diagnostic image startup observation. Wi-Fi join reached link state -3, authentication rejected; cause not proven |
| 2026-10-30 | S1-U19 | `8be851b`+ | Codex | PASS (host) | — | PASS (host, dashboard). Dashboard next-junction direction. No physical turn check |
| 2026-10-30 | S1-U20 | Not recorded | Not recorded | PENDING | — | Case only. Sensor dashboard ledger. Results recorded under S1-U21 |
| 2026-10-30 | S1-U21 | `8be851b` | Codex | PASS (host, build) | — | PASS (host, dashboard, build). Sensor dashboard ledger. Dashboard seen at port 8080; no physical sensor check |
| 2026-10-31 | S1-U22 | Not recorded | Not recorded | PASS (host) | — | PASS (host, dashboard). One-screen dashboard |
| 2026-10-31 | S1-U23 | In manifest | Codex | PASS (host) | — | PASS (host, dashboard). Subsystem health table revision |
| 2026-10-31 | S1-U24 | `8981d04`+ | Codex | PASS (host) | [2026-10-31_S1-U24/](logs/2026-10-31_S1-U24/) | PASS (host, dashboard). Obstacle history |
| 2026-11-02 | S1-U25 | `79c13f1`+ | Codex | PASS (host) | [2026-11-02_INT-139b/](../integration/logs/2026-11-02_INT-139b/) | PASS (host, dashboard). Subsystem sensor dashboards |
| 2026-11-02 | S1-U26 | `79c13f1`+ | Codex | PASS (host) | [2026-11-02_S3-U24/](../line_barcode/logs/2026-11-02_S3-U24/) | PASS (host, dashboard). Compact simultaneous subsystem overview |
| 2026-11-08 | S1-U14 | `2e254ff` | Nicholas, Claude | PASS (host, build) | — | Each of the 12 fault bits round-trips alone and together; undefined bits not sent; reserved bits rejected; v2 frames rejected by version. Course bss 83700 B. Not run on a real link (S1-M01) |

# Comms

## Contents

- [Scope](#scope)
- [Files](#files)
- [Contract](#contract)
- [Status](#status)
- [Diagnostic trace policy](#diagnostic-trace-policy)

## Scope

Owner: Shaun. WiFi, MQTT, telemetry, events and mapping requests. Comms runs as bounded steps in core-0 `io_task`, not as its own task. Only the WiFi service context calls CYW43, lwIP or MQTT; the transport is `lib/liblwip/sysdepend/pico_rp2040/mqtt_utk.c`.

Mission needs WiFi and MQTT before a local start. A disconnect during a run is report-only. Networking never starts, stops or drives the robot; mapping and snapshot requests are the only PC inputs.

## Files

| File | Job |
| --- | --- |
| `comms.h` | The subsystem's only header: wire frames, bridge, event log and task API |
| `comms.c` | `comms_step` task work, `comms_event`, telemetry gathering |
| `mqtt_frame.c` | Binary wire codec (host-tested) |
| `report.c` | Event log, report assembly, TX/RX bridge (host-tested) |
| `comms_test.c` | Firmware self-tests |

## Contract

| Path | Rule |
| --- | --- |
| Telemetry | Gathers coherent producer values; busy optional data is skipped, so work stays bounded |
| Events | 16-entry log under a cross-core mutex; a failed post keeps the event; a full log overwrites the oldest and counts the drop |
| Bridge | TX depth 4, RX depth 2, maximum frame 18 bytes; full or oversized frames are dropped; no waiting in WiFi context |
| Budgets | Two RX frames, two event posts and up to two of each trace per release |
| Mapping | Four distinct A-Z letters, installed by Mission only in READY or STOPPED; the ACK reports that outcome, not network receipt |
| Connection | Online after subscription succeeds; refusal, disconnect or timeout backs off; reconnect restores the subscription and never replays old actions |

Frames, topics, reserved bits and golden vectors: [MQTT protocol](../../README.md#mqtt-binary-protocol-schema-version-3). Fault policy: [fault bitmask](../../README.md#fault-policy). Credentials stay in ignored `config/wifi_secrets.h`.

Slow telemetry and broker acknowledgements do not prove live sensor progress, short transitions or brake timing. Retained online status can be stale.

## Status

Codec, report assembly, mapping path, event log and bridge are host-tested, and S1-U01 to S1-U05 pass on the car; results are in the [comms test log](../../testing/comms/TEST_LOG.md).

- Type 9 LINE trace remains available when `TRACE=1`, and is suppressed with other high-rate diagnostic frames by `TRACE=0`.

## Diagnostic trace policy

`TRACE=1` (test and trial images) retains high-rate WHEEL, LINE and BARS trace
frames for controlled debugging; the `full_course` image is `TRACE=0`. HUMP frames
are always sent, because the dashboard's tilt, XYZ and terrain graphs need them
on the full-course image: 10 per second on flat ground, 20 on a hump, dropped if
the bridge is full. `TRACE=0` removes their production and
transport while preserving 500 ms TELEMETRY, snapshots, EVENT, mapping and
acknowledgement frames. Both profiles use the same schema, safety and local
controls. In particular, TRACE=0 must not be confused with TEST=1 motor
inhibition. This is an explicit Comms choice: **keep debug frames available**,
and never in `full_course`.

Record the exact source commit, TRACE flag and UF2 SHA-256, then compare the
same number of physical laps and their actual results. Use `scripts/triage_mqtt_capture.py`
for saved MQTT monitor logs. High-rate diagnostics can affect scheduling,
but no causal impact is established until real matched trials pass.
A missing debug frame is not evidence of a missing normal EVENT.

# Subsystem 1: Communications

## Contents

- [Functionality](#functionality)
- [Interfaces and contract](#interfaces-and-contract)
  - [Connection states](#connection-states)
- [Data and fault bits](#data-and-fault-bits)
- [Status](#status)
- [Open items](#open-items)

## Functionality

Owner Shaun. [Plan](../../plan.md) owns work sequence;
[architecture](../architecture.md#tasks) owns target tasks/authority and
[team](../team.md) owns pins/people. Target S1 Reporting runs as bounded steps in
core-0 `io_task` in the working implementation; it is not a separate application task. Kernel WiFi/USB stay on
core 0. Only WiFi service context calls CYW43/lwIP/MQTT. Keep transport in
`lib/liblwip/sysdepend/pico_rp2040/mqtt_utk.c`.

Mission requires WiFi and MQTT before local start; disconnect during a run is
report-only. Network callbacks never start/stop/drive the robot. Mapping and
snapshot requests are the accepted PC inputs; no remote movement authority.

## Interfaces and contract

| Path | Existing code / target contract |
| --- | --- |
| Telemetry | `subsystem1.c` gathers coherent producer values; target skips busy optional data and bounds work |
| Events | `s1_event`, `s1_report.c`: 16-entry log under a cross-core mutex; insertion waits up to the existing 2 ms snapshot timeout, failure increments a saturating per-core diagnostic; oldest overwritten on full ring with a separate drop count |
| Bridge | `s1_bridge.c/.h`: TX depth 4, RX depth 2, maximum frame 23 bytes; full/oversized frames dropped; no waiting in WiFi context |
| Mapping | Four distinct A-Z letters; target bounded I/O-to-Mission hand-off, installed only READY/STOPPED, ACK after outcome |
| MQTT callbacks | Current command topic is matched before return; payload fragments are copied into bounded staging then RX. No retained callback pointers; any future multi-topic RX needs copied topic metadata |
| Initialization | Current rings are static storage; any replacement kernel buffers must be created/published before transport use |

Keep existing bounded rings provisionally; literal kernel message buffers are
unresolved D09. The supplied diagram defines separation, not a proven need to
rewrite the bridge. RX/event processing must have a fixed per-release budget;
the working `s1_step` limits RX to two frames and event posting to two per release.

### Connection states

Current transport connects, subscribes and declares online after subscription
success; refusal/disconnect/timeout backs off. Connection acceptance restores the
subscription. Retain newest data on reconnect rather than replaying old actions.
Credentials stay in ignored `config/wifi_secrets.h`; build profiles and monitor
commands are in [README](../../README.md#wireless-telemetry).

## Data and fault bits

The only protocol definition is [schema v2](../architecture.md#mqtt-binary-protocol-schema-version-2):
23-byte telemetry/snapshot, 11-byte event, 5-byte mapping, 3-byte ACK and 2-byte
snapshot request. Exact topics, layouts, reserved bits and golden vectors live
there. Preserve schema; new Mission state labels/codes need explicit compatibility
review even if byte lengths stay unchanged. Fault policy is owned by
[architecture](../architecture.md#fault-bitmask), not a second S1 table.

Slow telemetry and broker acknowledgements do not prove live sensor progress,
short transitions, brake timing or installed mapping. ACK is installation result,
not merely network receipt. Retained online status can be stale.

## Status

At verified remote main `5e6a45b`: binary codec, report assembly, mapping path,
event log and TX/RX bridge exist; Reporting is still a separate core-1 task.
That is the pinned baseline, not the working implementation: `s1_step` now runs
in I/O, the event log uses a cross-core mutex and mapping installation uses
`mission_mapping` hand-off/reply. Posting failure keeps the event; ring saturation
still overwrites oldest events and increments drops. Diagnostics use nonwaiting
optional snapshot copies. ACK OK follows Mission installation. Local dashboard/tuning checkpoint is distinct from remote main.
No physical network/mission success is inferred from code presence; consult
[S1 results](../../testing/subsystem1/TEST_LOG.md).

## Open items

See D09/D13/D14 in [pending decisions](../project-specs.md#pending-decisions).
Test mapping while stopped/moving, malformed/oversized/fragmented messages,
full RX/TX/event buffers, floods, startup order, reconnect subscriptions,
network readiness and mid-run loss. Verify all four firmware profiles and actual
Pico behavior separately; ensure diagnostics never delay Motion.

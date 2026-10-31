# Repository audit — 2026-10-31

## Contents

- [Scope](#scope)
- [Findings](#findings)
- [Verification](#verification)
- [Limits](#limits)

## Scope

Audited commit `5e63fe0e90776eb834fd3d27ba438a0f8e017d8e`, initially clean.
Reviewed application startup, Motion, Mission/Safety, all five subsystem
integration paths, shared data and GPIO interrupts, active RP2040 network/SMP
boundaries, laptop decoder/dashboard behavior, Make/Docker/CI and documentation
contracts. Detailed review concentrated on application and integration behavior;
imported kernel and inactive CPU ports were not reviewed line by line.
No production code, build configuration, commits or remote state changed.

## Findings

1. **[P2] Terrain accepts acceleration-dominated data as a valid hump.**
   `app_program/subsystem4/terrain_algo.c:97` derives pitch directly from filtered
   acceleration; lines 123–158 integrate and accept it without an acceleration
   magnitude, wheel-difference or steady-speed quality check. The
   `TERRAIN_QUALITY_*` settings have no consumers. The runtime wrapper checks
   encoder freshness/faults but provides no dynamic quality rejection.
   A synthetic level-chassis fore/aft acceleration/braking input with magnitude
   about 1.414 g produces **one valid hump with maximum code 44 (44 mm at the
   current scale)**. This is a host counterexample, not a measured vehicle trace.
   It violates the Terrain contract to reject acceleration-dominated estimates;
   it can corrupt both count and retained maximum and trigger unnecessary
   tracking slowdown. Gate/invalidate the whole encounter on unsupported dynamic
   evidence, keeping completed valid maxima. Calibrate physical limits through
   D06 rather than treating the existing proposed constants as measured limits.
   [Probe](terrain-dynamics.c), [observed output](terrain-dynamics.txt).

2. **[P2] Received direction requests survive connection loss.**
   `app_program/subsystem1/s1_bridge.c:87` flushes only outgoing frames. A queued
   type-7 request remains readable after `s1_bridge_link(false, false)`;
   `subsystem1.c` drains RX without checking its connection context, and
   `mission_queue_direction()` accepts it while FOLLOW_LINE continues offline.
   A request from the previous connection can therefore change a later junction
   after that connection is lost. The bridge probe confirms the offline delivery;
   downstream acceptance is established by source review, not a hardware run.
   Discard requests from a lost connection using the existing producer/consumer
   ownership rules; do not have the transport independently overwrite the RX
   consumer index. The receive callback in `mqtt_utk.c:131` also forwards payloads
   without retained-message filtering; broker replay is a related untested path.
   [Probe](bridge-disconnect.c), [observed output](bridge-disconnect.txt).

3. **[P2] Laptop decoding does not enforce the binary schema.**
   `scripts/mqtt_client.py:80` masks fields without rejecting reserved bits or
   invalid field combinations. `frame_type()` checks only version/type/length.
   The actual dashboard decoder accepts a telemetry frame with reserved bit 7
   set in byte 3 and the invalid range sentinel marked valid, displaying
   **8191 mm** as a measured range. Firmware's C decoder already rejects the
   reserved bit, so the two consumers disagree on accepted frames. Reject
   malformed telemetry/events before displaying data or updating speed history;
   add negative vectors alongside the golden-vector checks.
   [Probe](decoder-validation.py), [observed output](decoder-validation.txt).

## Verification

Existing checks executed successfully against the exact source commit:

- `sh testing/pure/run.sh`: PASS (C policy/codec/sensor suites, concurrent
  shared-data/mapping/bridge checks and Python decoder self-test).
- `node testing/pure/test_dashboard.mjs`: PASS.
- `python3 testing/pure/test_mqtt_cli.py`: PASS.
- `python3 testing/pure/test_dashboard_http.py`: PASS on authorized localhost
  retry. Initial sandbox execution could not bind a socket (PermissionError);
  this was an environment restriction, not a failed assertion.
- All three new defect probes: reproduced their asserted current behavior.
- Tracked filename scan: no `.elf`, `.uf2`, `.map`, `.mp4` or real secrets header
  tracked; `config/wifi_secrets.example.h` is the tracked example.

The earlier INT-44 findings are fixed in the current source and were not
re-reported. Regression outputs were observed in this chat; the summary here
is not a saved full console transcript. New probe outputs are saved verbatim.
The [manifest](manifest.json) records source hashes and the exact commit.

## Limits

No new firmware builds (native cross-compiler/SDK not configured), flashing,
firmware execution, live broker publishes, physical sensors/motion, latency,
stack margins or floor trials. Existing INT-45 builds are historical evidence,
not fresh results from this audit. No fresh visual/browser review.
Physical bypass remains unreleased; provisional geometry, polarity, gains,
timing and disabled stall detection remain documented trial settings.
This audit does not establish physical readiness or exhaustively certify the
imported kernel/SDK and inactive processor ports.

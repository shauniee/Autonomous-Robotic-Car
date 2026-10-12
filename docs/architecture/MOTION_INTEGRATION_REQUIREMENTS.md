# Motion integration requirements

Phase 4B: documentation only. This contract describes the S2 API after Phase 4A (`26464dc`) and the work required from other subsystem owners. The current S3 task publishes IR snapshots; it does not publish motor requests or stop Motion. IR-guided movement requires that producer hookup and working motor hardware.

## Ownership and current API

S2 exclusively writes motor outputs and owns request validation, expiry, persistent stop, reset and service health. S3 owns sensor acquisition, calibration, line classification, steering conversion and local start/reset controls. Keep IR types and decisions outside S2. The runtime changes required below belong to their respective owners; none are implemented in this phase.

Include [subsystem2.h](../../app_program/subsystem2/subsystem2.h) for the generic API. [motion_policy.h](../../app_program/subsystem2/motion_policy.h) defines the request and bounds.

| API / field | Current contract |
| --- | --- |
| `MotionRequest.left`, `.right` | Finite forward PWM fractions in inclusive range 0..0.35. These are duties, not measured wheel speeds. Reverse is unavailable. |
| `.sampled_ms` | Original acquisition time in the monotonic millisecond clock `tk_get_otm().lo`. Unsigned sample age <=60 ms is accepted; >60 ms is expired. |
| `.enabled` | True only when the producer authorizes movement. False requests braking but does not itself latch persistent stop. |
| `s2_publish(request)` | Copies the latest request through a mutex; requests are not queued. Check `ER < E_OK`. Success means copied, not accepted at output or physically executed. An enabled invalid/expired request brakes and latches stop when serviced. |
| `s2_stop()` | Latches stop and independently wakes Motion, without taking the request mutex. Each call invalidates any previous brake acknowledgement. S2 boots with stop latched. |
| `s2_health(&health)` | Check the return code before consuming fields. `stopped` reports inhibition; `brake_acknowledged` reports successful brake service for the latest stop. `sequence` and `serviced_ms` report successful output service. None prove physical stationarity or speed. |
| `s2_reset_stop()` | Returns `E_OBJ` unless stop is latched and its latest brake service acknowledged. Success clears cached requests and invalidates copies from before reset. Reset never commands movement. Other failures must also be handled. |

Use these calls from the authorized application producer task on core 1, after S2 initialization. Runtime stop/reset serialization relies on sharing Motion's core; cross-core runtime stop/reset is unsupported. Calls that acquire mutexes belong in task context, outside interrupt handlers or interrupt-masked sections. Motion normally services every 20 ms and can wake for stop; no measured stop-latency bound is established.

## Required S3 integration

1. Keep one authorized motor-request producer on core 1. Extend S3's existing task or place its adapter in S3; preserve the public snapshot interface and sensor math. Snapshot publication alone cannot drive S2.
2. Keep motion disabled during calibration/capture and until a fresh local start action. Before publishing enabled duties, require a present, valid, fresh acquisition, valid calibration and guidance, and an agreed visible-line condition. Inspect validity flags before using steering; invalid snapshot fields can be zero.
3. Convert steering into bounded forward duties. A proposed initial mapping for normalized steering in -1..1 is `left=0.10+0.10*steering`, `right=0.10-0.10*steering`, giving 0..0.20. Validate finite inputs and confirm physical steering sign/polarity before adopting it. This mapping is a proposal, not current motor behavior.
4. Preserve the acquisition timestamp when reusing a sample. Never replace it with publication time. Publish at a cadence compatible with the 60 ms expiry; S3 currently samples every 20 ms and services inputs every 10 ms.
5. Route the local stop action, lost/unknown/stale/invalid sensing, publication failures and producer error exits through `s2_stop()`. Check its result and remain locally inhibited on errors. Stop-on-line-loss is the proposed initial behavior; recovery requires a separate agreed policy.
6. Stop publishing enabled requests while stopped. After a successful health read reports `stopped && brake_acknowledged`, allow a fresh local reset action to call `s2_reset_stop()`. Its return code is authoritative because another stop may invalidate the observed acknowledgement. On failure remain stopped; do not reset automatically each sample.
7. After successful reset, require a separate fresh start action and revalidate guidance before publishing a new enabled request. Held buttons, boot inputs and cached commands must not replay a start. Quiesce old producer work: S2's internal epoch rejects copied pre-reset work, but the legacy request has no producer-generation identifier to reject every delayed new publication.

GP20/GP21 currently serve capture/cancel behavior in S3. Assigning them start/reset and motor-stop semantics requires an explicit S3 change that preserves calibration behavior; those semantics are not wired by this document.

## Requirements for other owners

Mission and Obstacle owners must agree command ownership and handover before becoming producers. Do not race multiple writers or bypass S2 to write PWM. S1 telemetry must distinguish software service, stop latch and brake acknowledgement from measured motion. Terrain/Obstacle consumers needing physical stationarity require later feedback or owner measurements; brake acknowledgement alone is insufficient.

The [older IR handoff](IR_MOTION_HANDOFF.md) contains historical publication/button examples and Phase 3 reset limitations. Use this document and the current API for post-4A reset behavior; the historical examples do not establish a current S3 integration.

## Open decisions and review boundary

- Confirm Nicholas owns the producer hookup and whether it lives directly in the IR task or a separate S3 adapter.
- Agree the GP20 calibration/reset/start transitions and GP21 stop handling, including whether reset preserves calibration.
- Confirm steering sign, safe initial duty and motor wiring/polarity once the hardware fault is resolved.
- Is stop-on-line-loss sufficient for the initial robot, or is bounded recovery required by the team?
- Which owner will coordinate command handover when Mission/Obstacle integration begins?

Phase 4B supplies requirements only. No S3 or S2 runtime code changes, tests, test logs, flashing or physical trials are included. `APP_TEST` builds remain motor-inhibited. Physical motor trials require the IR hookup, resolved hardware and the separately approved Phase 5 owner/team milestone. Stop for review before starting that phase.

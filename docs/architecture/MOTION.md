# Motion

Owner: Mahesha / S2. Parent: [system architecture](../ARCHITECTURE.md). This guide preserves the full-robot encoder/PID proposal. The existing duty-only bench API is specified in the [IR handoff](IR_MOTION_HANDOFF.md#current-interface-contract); it is not an encoder-action implementation. Operational status is in [GETTING_STARTED](../GETTING_STARTED.md#current-capabilities).

## Functionality

Motion is the sole motor-output writer. The motor driver controls two H-bridges; quadrature encoders provide direction/counts for speed, distance and turn estimates. Planned per-wheel PID closes measured speed loops, corrects straight travel and completes forward/backward distance or left/right/U-turn actions. Line Tracking supplies calibrated P guidance; it does not replace wheel-speed PID or write motor PWM.

Mission sends leased targets. Motion checks local stop/fault and expiry before output, publishes coherent measured feedback, and retains action completion until acknowledged. Safety can wake Motion independently of the request mutex.

## State diagram

The existing full-design states are organized below; transition labels express the established lease/action rules.

```mermaid
stateDiagram-v2
    [*] --> Initializing
    Initializing --> IdleBraked: Setup valid
    Initializing --> Inhibited: Setup failed
    IdleBraked --> SpeedControl: Fresh speed target
    IdleBraked --> Maneuver: Fresh action ID
    SpeedControl --> IdleBraked
    SpeedControl --> Maneuver: New action
    Maneuver --> Completed: Target reached
    Completed --> IdleBraked: Acknowledged
    SpeedControl --> Inhibited: Stop fault or expiry
    Maneuver --> Inhibited: Stop fault stall or deadline
    Completed --> Inhibited: Stop or fault
    IdleBraked --> Inhibited: Stop or fault
    Inhibited --> IdleBraked: Cause clear and local reset
```

## States and responses

| State | Actuation / response |
| --- | --- |
| Initializing | Set electrical brake before driver/encoder configuration; unavailable required setup inhibits movement |
| Idle Braked | Zero demand/brake; wait for a fresh valid command |
| Speed Control | Regulate each measured wheel speed with planned PID; renew bounded lease; zero demand brakes |
| Maneuver | Track one ID-bound distance/angle target with encoder completion and absolute timeout |
| Completed | Brake at target; retain completed ID/status until acknowledged through the following command context |
| Inhibited | Brake both channels, invalidate previous target and retain reason; reset alone cannot move |

The diagram's unlabeled Speed Control → Idle Braked return means zero demand. The current bench services forward duty requests, not these full states. TEST firmware always brakes. Existing stop/reset serialization limitations remain explicitly preserved in the handoff.

## GPIO and peripherals

Use [TEAM's motor/encoder allocations](../TEAM.md#2-pins) and [PWM ownership](../TEAM.md#3-peripherals). [TECHNICAL_INFO](../TECHNICAL_INFO.md#motors-and-encoders) owns connector definitions, drive truth table and distance/yaw formulas.

## Modules and drivers

| Module | Purpose / status |
| --- | --- |
| `subsystem2.c/.h` | Existing lifecycle, motor service, request/health mutex, persistent stop event and public `MotionHealth` |
| `motion_policy.c/.h` | Existing duty/freshness/stop validation and `MotionRequest` |
| `drv_motor.c/.h` | Existing exclusive PWM/H-bridge boundary |
| Suggested encoder driver | Bounded quadrature edge capture; coherent counts, direction and acquisition time |
| Suggested speed estimator / wheel controller | Count-window speed/distance conversion and separate per-wheel PID |
| Suggested action controller | Unique action IDs, lease/deadline validation, completion retention, acknowledgement and stall detection |

Use [shared `M_motion_cmd` / feedback contracts](../ARCHITECTURE.md#shared-data-rules) for full integration. Feedback must expose measured counts/speeds/distance, validity/time, readiness/stationarity and active/done/failed action ID/reason. The existing bench's software health sequence is insufficient for scan stationarity.

## Business logic

- Calibrate decoded counts per wheel revolution, geometry, polarity and speed before implementing physical actions; unresolved values are in [PROJECT_SPECS](../PROJECT_SPECS.md#pending-decisions).
- Run measured wheel-speed PID separately from memoryless P line guidance. Bound requested speed/output and handle stalled encoders, saturation and invalid feedback explicitly.
- A renewed lease preserves the action ID and absolute deadline; repeated commands cannot restart or duplicate a turn. An expired/unconfigured target brakes locally.
- Boot, local stop, collision, stall, critical fault and deadline failure inhibit output. Reset discards prior targets; a new Mission command is required afterward.
- Measure commanded brake at outputs and physical stopping separately. LOW/LOW is the selected electrical brake; it does not establish stopped wheels.

Verification belongs to [S2](../../testing/subsystem2/TEST_LOG.md), [integration](../../testing/integration/TEST_LOG.md) and [system](../../testing/system/TEST_LOG.md) records. Planned T02/T05/T14/T15 cover stop/actions/fault/load; no encoder action or physical PID acceptance is claimed.

# Subsystem 2: Motion

Owner: Mahesha. S2 alone writes motor PWM and owns the encoder capture/control task. [TEAM](../TEAM.md) owns pins/PWM resources; [TECHNICAL_INFO](../donotsubmit/TECHNICAL_INFO.md#motors-and-encoders) owns component facts and conversion formulas.

## Control and contracts

Quadrature ISR capture validates transitions, records signed wrapping counts/epoch/faults, and runs before barcode/Echo dispatch. Raw encoder quiet-count validity is independent of calibrated wheel displacement. Calibrated feedback includes wheel distance/speed, encoder-derived pose/yaw, outputs, action identity/status and stationarity after 60 ms of quiet counts.

`MotionCommand` supports brake, signed wheel speeds, signed distance and signed turn. It contains action/run identity, original required-data time, issue/expiry and an immutable absolute deadline. A 60 ms lease renewal cannot move the original action origin/target/deadline. Distance and turn completion require both wheels within tolerance; the reached wheel can brake while its partner finishes. Completion/failure remains until matching acknowledgement.

Wheel PID uses signed feedback and bounded output with conditional integral anti-windup. Invalid timing, stale input, illegal encoder capture, stall, expired action or invalid command brakes/faults. Freshness uses wrap-safe time; old actions cannot replay across reset epochs.

Non-finite wheel distances cannot establish an action origin or produce distance/turn output. Polling command/feedback copies follow the [shared lock policy](../TEAM.md#5-interfaces-between-subsystems); independent stop does not acquire that mutex.

| API | Purpose |
| --- | --- |
| `s2_command` | Submit validated leased movement |
| `s2_feedback_copy` | Nonblocking coherent measured/raw feedback |
| `s2_ack` | Acknowledge retained matching completion |
| `s2_stop` | Independent persistent stop, separate from request locking |
| `s2_prepare_reset` / `s2_reset_stop` | Brake-acknowledged reset/release; invalidate old action context |
| `s2_health` | Actual service progress, stop/ack and full-width epoch |

Legacy `s2_publish` returns E_NOSPT. Runtime stop/reset calls stay on core 1. TEST always forces braking regardless of Mission's internal moving state.

## Configuration and verification

Measured counts/metre, polarity, geometry, PID, speeds, tolerances, stall limits and actual stop latency remain required. Optional commissioning uses a locally selected electrical release, raised wheels and a two-second duty/speed action; it defaults disabled and excludes mission readiness. [GUIDE](../../GUIDE.md) owns operation/power policy.

Modules: `drv_encoder`, `drv_motor`, `motion_policy`, `motion_control` and lifecycle/task `subsystem2`. [S2 cases](../../testing/subsystem2/TEST_LOG.md) distinguish policy fixtures from motor/encoder and accuracy measurements.

## Provisional wheel measurements

The operator reported 65 mm wheel diameter and 113 mm centre-to-centre track. Slow USB-only manual turns produced provisional left/right counts of 2530.5/2526.5 per revolution and forward signs of -1/-1. These values are entered with calibration confirmation flags still false. The earlier fast trial had four illegal transitions and is rejected; slow left repeats varied. [Raw provenance and calculations](../../testing/subsystem2/logs/2026-10-15_wheel-measurements.json) require repeat marked turns and measured ground travel before acceptance.

[Electrical status](../../testing/subsystem2/logs/2026-10-15_electrical-status.json) records the disconnected GP28 sensing jumper and unknown motor/supply ratings. No powered tuning or braking result is claimed. Explicit two-second raised-wheel commissioning can use IR-only calibration without the missing IMU, but still requires verified electrical release; it defaults disabled.

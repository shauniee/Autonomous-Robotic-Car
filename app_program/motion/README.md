# Motion

## Contents

- [Scope](#scope)
- [Files](#files)
- [Contract](#contract)
- [Wheel-speed tuning](#wheel-speed-tuning)
- [Status and open items](#status-and-open-items)

## Scope

Owner: Mahesha. Motion runs on core 1, outranks Mission and is the only writer of GP8-GP11. It owns per-wheel PID, encoders and the bounded FOLLOW, DISTANCE and TURN modes. Other subsystems supply guidance and never write PWM. Movement, stop and freshness rules: [Motion command contract](../../docs/architecture.md#motion-command-contract). Wiring: [team](../../docs/team.md#2-pins).

## Files

| File | Job |
| --- | --- |
| `motion.h` | The subsystem's only header: encoder arithmetic, controller, task and driver APIs |
| `motion.c` | Task, motor and encoder drivers, wheel trace ring |
| `motion_control.c` | Controller: modes, ramps, wheel mix, PID (host-tested) |
| `motion_test.c` | Firmware self-tests |

## Contract

| API | Meaning |
| --- | --- |
| `motion_command` | Latest packed command under a private mutex; Mission is the only producer; not a route queue |
| `motion_feedback` | Status and action ID; match DONE or FAILED to the current action; repeats do not restart it |
| `motion_stop` | Persistent flag and wake-up, independent of the command mutex |
| `motion_health` | Coherent service progress and software brake acknowledgement; check the copy result first |
| `motion_reset_stop` | Invalidates the previous command; needs an eligible serviced stop; never starts movement |
| `motion_inhibit_startup` | Startup-only inhibit; TEST images always brake |

- GP21 is read before the command copy and again inside the output critical section. Motion latches faults, expiry and failed actions itself. GP21 release resets only after the cause clears and the brake is serviced; a separate GP20 press starts.
- A command's sample time cannot be refreshed by copying. Stationarity comes from measured encoders, not zero demand or a software acknowledgement.
- **FOLLOW** ramps the common base, then steering, then mixes left `v(1+q/127)` and right `v(1-q/127)`, so the robot centre keeps the commanded speed. STOP, stale or zero-speed requests bypass both ramps.
- **TURN** style 0 is a pivot, 1 a single-wheel swing and 3 a ramped pivot; style 2 is rejected. DISTANCE style 3 is a ramped approach.

Packed widths: [data table](../../docs/project-specs.md#5-data-representation-table-week-13). Keep wrapping counts with per-consumer origins.

## Wheel-speed tuning

Mission chooses steering, Motion mixes wheel targets and each encoder closes its wheel's PID. Keep steering constants fixed while tuning wheel response. Use `mqtt_client.py monitor` on the battery-powered normal robot with USB disconnected, following the [power policy](../../README.md#power-and-flashing). `WHEEL` JSON gives `sampled_ms`, `target_left/right`, `measured_left/right` in mm/s, `duty_left/right`, `inhibited` and `output_ok`. Duty is the software command, not measured PWM or torque. TEST images show zero duty and cannot tune loaded motors.

1. Verify forward direction, encoder signs and rolling scale before gains, per wheel.
2. Establish raised-wheel behaviour and stop response, then loaded travel. Use stable equal targets first, then unequal targets and low inner-wheel speeds.
3. Set `MOTION_KS` and `MOTION_KF` from measurements. Tune `MOTION_KP` for response and `MOTION_KI` for persistent error, one change per recorded trial, with `MOTION_KD` zero. Do not raise the duty cap without motor and driver ratings.
4. If measured speed oscillates under constant targets, look at wheel control, encoder noise, friction and power. If targets oscillate and wheels follow, look at IR and steering.

The [wheel trace](../../docs/architecture.md#wheel-trace-15-bytes) is best effort: timestamp gaps show missing releases. Neither a graph nor a host test is a physical pass.

## Status and open items

Controller and actions are host-tested; results are in the [motion test log](../../testing/motion/TEST_LOG.md). A hung Motion or kernel stays unprotected by these software checks.

- Verify encoder signs and forward direction with raised wheels; never flip a sign to match source.
- Calibrate polarity, counts, wheel and track geometry; verify distance and 90/180 degree turns, one wheel on a hump, stall and GP21 (D03, D07, D08).
- Check the motors hold 200 mm/s on the outer wheel, which the 100 mm/s course speed needs at full steering.
- Measure GPIO IRQ edge load, sensor-to-output latency, brake latency and stopping distance. Trials: MR-14..25, T02, T05, T15.

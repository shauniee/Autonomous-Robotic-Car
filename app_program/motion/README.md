# Motion

## Contents

- [Scope](#scope)
- [Files](#files)
- [Contract](#contract)
- [Wheel-speed tuning](#wheel-speed-tuning)
- [PID tuning report](#pid-tuning-report)
- [Motion accuracy evaluation](#motion-accuracy-evaluation)
- [Status](#status)

## Scope

Owner: Mahesha. Motion runs on core 1, outranks Mission and is the only writer of GP8-GP11. It owns per-wheel PID, encoders and the bounded FOLLOW, DISTANCE and TURN modes. Other subsystems supply guidance and never write PWM. Movement, stop and freshness rules: [Motion command contract](../../README.md#motion-command-contract). Wiring: [team](../../team.md#2-pins).

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
- **Speed profile:** every planned move starts and stops like a lift, with acceleration limited by `MOTION_ACCEL_MMPS2` and its rate of change by `MOTION_JERK_MMPS3`. Lower either in `robot_config.h` for a gentler move, at the cost of more stopping distance. Safety stops skip the profile. Other subsystems call `motion_stop_distance_mm(speed)` for the travel of a planned stop, instead of assuming a car stops where a command is given.
- **TURN** style 0 is a pivot, 1 a single-wheel swing and 3 a pivot that keeps its speed from a FOLLOW hand-over; style 2 is rejected. DISTANCE style 3 does the same.

Packed widths: [data table](../../team.md#6-data-representation). Keep wrapping counts with per-consumer origins.

## Wheel-speed tuning

Mission chooses steering, Motion mixes wheel targets and each encoder closes its wheel's PID. Keep steering constants fixed while tuning wheel response. Use `mqtt_client.py monitor` on the battery-powered normal robot with USB disconnected, following the [power policy](../../README.md#power-and-flashing). `WHEEL` JSON gives `sampled_ms`, `target_left/right`, `measured_left/right` in mm/s, `duty_left/right`, `inhibited` and `output_ok`. Duty is the software command, not measured PWM or torque. TEST images show zero duty and cannot tune loaded motors.

1. Verify forward direction, encoder signs and rolling scale before gains, per wheel.
2. Establish raised-wheel behaviour and stop response, then loaded travel. Use stable equal targets first, then unequal targets and low inner-wheel speeds.
3. Set `MOTION_KS` and `MOTION_KF` from measurements. Tune `MOTION_KP` for response and `MOTION_KI` for persistent error, one change per recorded trial, with `MOTION_KD` zero. Do not raise the duty cap without motor and driver ratings.
4. If measured speed oscillates under constant targets, look at wheel control, encoder noise, friction and power. If targets oscillate and wheels follow, look at IR and steering.

The [wheel trace](../../README.md#wheel-trace-15-bytes) is best effort: timestamp gaps show missing releases. Neither a graph nor a host test is a physical pass.

## PID tuning report

Tuned on raised wheels, then checked loaded on the floor (S2-U04).

| Trial | Change | Observed |
| --- | --- | --- |
| Raised wheels | Feedforward fit | Left 1.05e-4, right 0.95e-4 over 60 to 200 mm/s (`robot_config.h`) |
| Raised wheels | 100 mm/s step with fitted feedforward | Peak 112 mm/s; wheels alternate below 60 mm/s at KP 0.0003 |
| Raised wheels | KP lowered to 0.0001 | The alternation is gone |
| Floor, battery (S2-U04) | `straight_1m` at 100 mm/s | Mean L 100.2 / R 100.3 mm/s, SD 3.6 mm/s (one count step is 4.3 mm/s), range 90-116 mm/s, duty L 14-17 / R 15-18; no oscillation |

Final values: `MOTION_KS` 0.05, `MOTION_KF` 0.0001, `MOTION_KP` 0.0001, `MOTION_KI` 0.0010, `MOTION_KD` 0.

## Motion accuracy evaluation

Results on the car, recorded in the [motion test log](../../testing/motion/TEST_LOG.md).

| Move | How measured | Runs | Result |
| --- | --- | --- | --- |
| Straight 100 mm (S2-U02) | Taped end point | 5 | 5/5 stopped at 100 mm, 0 mm lateral error; encoders 98.2-100.2 mm |
| Straight 1 m (S2-U01, S2-U04) | Encoders | 2 | 998 mm and 995 mm |
| Pivots: 90 left, 90 right, 180 (S2-U03) | Tape against a square; encoders | 21 | Heading error about 5 degrees; encoders left 88.1-90.7, right -88.4 to -88.9, 180 turn 178.4-179.8 degrees |
| Stop at a box (SYS-U01) | Sonar `front=` at rest | 2 | 142 and 140 mm against a 150 mm target |

## Status

Controller and actions are host-tested, and S2-U01 to S2-U05 pass on the car; results are in the [motion test log](../../testing/motion/TEST_LOG.md).


# Motion

## Contents

- [Scope](#scope)
- [Files](#files)
- [Contract](#contract)
- [Wheel-speed tuning](#wheel-speed-tuning)
- [PID tuning report (draft)](#pid-tuning-report-draft)
- [Motion accuracy evaluation (draft)](#motion-accuracy-evaluation-draft)
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
- **Speed profile:** every planned move starts and stops like a lift, with acceleration limited by `MOTION_ACCEL_MMPS2` and its rate of change by `MOTION_JERK_MMPS3`. Lower either in `robot_config.h` for a gentler move, at the cost of more stopping distance. Safety stops skip the profile. Other subsystems call `motion_stop_distance_mm(speed)` for the travel of a planned stop, instead of assuming a car stops where a command is given.
- **TURN** style 0 is a pivot, 1 a single-wheel swing and 3 a pivot that keeps its speed from a FOLLOW hand-over; style 2 is rejected. DISTANCE style 3 does the same.

Packed widths: [data table](../../docs/team.md#6-data-representation). Keep wrapping counts with per-consumer origins.

## Wheel-speed tuning

Mission chooses steering, Motion mixes wheel targets and each encoder closes its wheel's PID. Keep steering constants fixed while tuning wheel response. Use `mqtt_client.py monitor` on the battery-powered normal robot with USB disconnected, following the [power policy](../../README.md#power-and-flashing). `WHEEL` JSON gives `sampled_ms`, `target_left/right`, `measured_left/right` in mm/s, `duty_left/right`, `inhibited` and `output_ok`. Duty is the software command, not measured PWM or torque. TEST images show zero duty and cannot tune loaded motors.

1. Verify forward direction, encoder signs and rolling scale before gains, per wheel.
2. Establish raised-wheel behaviour and stop response, then loaded travel. Use stable equal targets first, then unequal targets and low inner-wheel speeds.
3. Set `MOTION_KS` and `MOTION_KF` from measurements. Tune `MOTION_KP` for response and `MOTION_KI` for persistent error, one change per recorded trial, with `MOTION_KD` zero. Do not raise the duty cap without motor and driver ratings.
4. If measured speed oscillates under constant targets, look at wheel control, encoder noise, friction and power. If targets oscillate and wheels follow, look at IR and steering.

The [wheel trace](../../docs/architecture.md#wheel-trace-15-bytes) is best effort: timestamp gaps show missing releases. Neither a graph nor a host test is a physical pass.

## PID tuning report (draft)

Draft from existing evidence only. All of it is on raised wheels, so it does not show loaded behaviour.

| Trial | Change | Observed |
| --- | --- | --- |
| 2026-11-01 | Feedforward fit, raised wheels | The log row names 60, 100 and 140 mm/s; `robot_config.h` says 60 to 200 mm/s and gives left 1.05e-4, right 0.95e-4. The range needs checking against the saved evidence |
| 2026-11-01 | 100 mm/s step with fitted feedforward | Peak 112 mm/s; wheels alternate below 60 mm/s at KP 0.0003 |
| 2026-11-01 | KP lowered to 0.0001 | The alternation is gone |

Current values, all provisional (D07): `MOTION_KS` 0.05, `MOTION_KF` 0.0001, `MOTION_KP` 0.0001, `MOTION_KI` 0.0010, `MOTION_KD` 0.

Missing before this is a report: loaded response on the floor, settling time and steady error per wheel, and a repeat with the final mechanics.

### Measurements to collect

Record each run separately with source SHA, UF2 hash, setup and evidence. Limits stay pending until agreed.

| Test | Attempts | Controlled setup | Record | Acceptance |
| --- | --- | --- | --- | --- |
| MR-14 encoder travel/reversals | 3 per wheel and speed | Wheel raised; measured revolution | Counts, direction, missed/invalid edges | Agree D03 |
| MR-21 1 m straight | 5 | Floor, unchanged firmware and battery | Actual distance, lateral error, wheel WHEEL traces | Agree D07 |
| MR-22 left/right 90°, U-turn 180° | 5 each | Same profile, measured mark | Heading errors and stop position | Agree D07 |
| MR-23 one-wheel hump turn | 5 each type | Known ramp and loading | Change in heading error | Agree D07 |
| MR-24 GP21 stop | Repeated | Controlled speed, clear run-out | Brake-output latency; physical stopping distance | Agree D08; evaluate 50 ms proposal |
| MR-25 motor stall | Repeated | Guard-enabled acceptance image only | Detection delay, latched fault and reset | Agree D07 |

## Motion accuracy evaluation (draft)

Nothing is measured yet, so this holds the method only. Tape each result on the floor and record the commit in the [test log](../../testing/motion/TEST_LOG.md).

| Move | How to measure | Runs | Result |
| --- | --- | --- | --- |
| Straight 500 mm | Taped distance against the commanded distance | 5 | Not measured |
| Left and right pivot 90 degrees | Heading error against a square | 5 each | Not measured |
| Stop 150 mm from a board | Taped rest distance | 5 | Not measured |

## Status and open items

Controller and actions are host-tested; results are in the [motion test log](../../testing/motion/TEST_LOG.md). A hung Motion or kernel stays unprotected by these software checks.

- Verify encoder signs and forward direction with raised wheels; never flip a sign to match source.
- Calibrate polarity, counts, wheel and track geometry; verify distance and 90/180 degree turns, one wheel on a hump, stall and GP21 (D03, D07, D08).
- Check the motors hold 200 mm/s on the outer wheel, which the 100 mm/s course speed needs at full steering.
- Measure GPIO IRQ edge load, sensor-to-output latency, brake latency and stopping distance. Trials: MR-14..25, T02, T05, T15.

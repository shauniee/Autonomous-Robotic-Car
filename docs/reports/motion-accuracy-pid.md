# Motion accuracy and wheel PID evaluation — evidence worksheet

**Status:** Draft procedure only. No new physical test or acceptance result is claimed.
Owner: S2 (Motion). Requirements: MR-14/15/19–25, D03/D07/D08, pending.md sections 1–2.

## Controller actually implemented

The Motion service is the only writer of motor PWM. It receives a packed
`MotionCommand` from Mission and uses encoder speed feedback in a 20 ms
release. For each wheel, the kernel-free `wheel_duty` calculation in
`app_program/motion/motion_control.c` combines static-friction feedforward
`MOTION_KS`, speed feedforward `MOTION_KF`, proportional error
`MOTION_KP`, accumulated integral `MOTION_KI`, and a derivative of
measured speed `MOTION_KD`. The result is clamped to
`MOTION_MAX_DUTY`, with conditional anti-windup. Following cannot command
power in the opposite direction merely to correct speed overshoot.

`WheelPid` stores two binary32 values per wheel (`integral` and
`previous_speed`); numeric range and fixed-point alternatives remain
unapproved in D07. A command being stopped or a controller output of zero
does **not** demonstrate that the physical chassis has stopped.

## Method and record

Do not drive until continuity, motor polarity, encoder signs and the GP21
stop response are checked with the prescribed safe power procedure.

| Test | Attempts | Controlled setup | Record | Acceptance |
| --- | --- | --- | --- | --- |
| MR-14 encoder travel/reversals | 3 per wheel and speed | Wheel raised; measured revolution | Counts, direction, missed/invalid edges | Agree D03 |
| MR-21 1 m straight | 5 | Floor, unchanged firmware and battery | Actual distance, lateral error, wheel WHEEL traces | Agree D07 |
| MR-22 left/right 90°, U-turn 180° | 5 each | Same profile, measured mark | Heading errors and stop position | Agree D07 |
| MR-23 one-wheel hump turn | 5 each type | Known ramp and loading | Change in heading error | Agree D07 |
| MR-24 GP21 stop | Repeated | Controlled speed, clear run-out | Brake-output latency; physical stopping distance | Agree D08; evaluate 50 ms proposal |
| MR-25 motor stall | Repeated | Guard-enabled acceptance image only | Detection delay, latched fault and reset | Agree D07 |

Record each run separately in `testing/system/TEST_LOG.md`, including
source SHA, UF2 hash, image flags, battery/setup, error and evidence links.
Do not average away individual failures.

## Interpretation still required

1. Compare encoder target, measured counts/s, applied duty and actual speed.
2. Check anti-windup and overshoot under loaded versus raised-wheel conditions.
3. Investigate encoder glitches independently of MQTT arrival-time speed
   estimates; compare raw wheel traces, counts and video.
4. Tune only one parameter per repeated trial, then rerun the complete
   required battery/lighting conditions.
5. Record numeric results and owner sign-off here **after** physical trials.

## Results

**Not yet measured on the current final firmware.** Do not mark MR-21/22/23/24
PASS from a host test or screenshot.

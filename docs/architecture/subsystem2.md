# Subsystem 2: Motion

## Contents

- [Functionality](#functionality)
- [Interfaces and contract](#interfaces-and-contract)
- [Data and fault bits](#data-and-fault-bits)
- [Wheel-speed tuning](#wheel-speed-tuning)
- [Status](#status)
- [Open items](#open-items)

## Functionality

Owner Mahesha. [Plan](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117) owns implementation sequence;
[architecture](../architecture.md#motion-command-contract) owns movement/stop/
freshness and [team](../team.md#2-pins) owns wiring. Target Motion stays core 1,
outranks Mission and is the sole writer of GP8-GP11. Numeric target scheduling is
pending D13. Reuse per-wheel PID, encoders and bounded FOLLOW/DISTANCE/TURN modes;
S3 supplies line guidance and never writes PWM.

## Interfaces and contract

Current source APIs in `subsystem2.h`, `motion_control.h`, `motion_health.h`:

| API | Meaning / target constraint |
| --- | --- |
| `s2_command` | Latest packed command under private mutex; Mission is sole producer; not a route queue |
| `s2_feedback` | Motion-written status/action ID; match DONE/FAILED to the current action; repeats do not restart it |
| `s2_stop` | Persistent event flag and wake-up independent of command mutex; runtime caller on core 1 |
| `s2_health` | Coherent service progress and software brake acknowledgement; check copy result first |
| `s2_reset_stop` | Invalidates previous command; requires eligible serviced stop; never starts movement |
| `s2_inhibit_startup` | Startup-only inhibit; TEST always brakes |

Target checks GP21 independently every Motion release before outputs. GP21 release
can return READY only after cause clearance and serviced brake; a separate fresh
GP20 starts. No stale target replay. Terrain on core 0 publishes impact evidence,
not an unsupported cross-core stop/reset call. Stop/reset precedence is independent
of telemetry or normal command-copy contention.

The command's original sample time cannot be refreshed by copying. Active turn/
distance actions need fresh Mission progress/encoder health without requiring a
visible line throughout the turn. Tracking needs fresh IR. Exact fields/timeouts
must be agreed in architecture before source changes. Stop/fault/deadline cancels
action; a new deliberate start is needed after reset. Stationarity comes from
measured encoders, not zero demand or a software acknowledgement.

## Data and fault bits

[Specs](../project-specs.md#5-data-representation-table-week-13) owns packed widths,
units, invalid codes and overflow behavior; [architecture](../architecture.md#fault-bitmask)
owns policy. Current command/health structures are packed/accessor-based, not the
old float duty-only API. Preserve wrapping counts and per-consumer origins.
Measured signs, counts/revolution, geometry, gains and stopping distances remain
D03/D07/D08. Do not repeat changing duty/speed constants here.

## Wheel-speed tuning

IR supplies digital line guidance; Mission chooses steering, Motion mixes wheel
speed targets, and each wheel's encoder closes its speed PID. Encoder feedback
cannot locate the line. Keep the steering/mixing constants fixed while tuning
wheel-speed response; IR needs reliable black/white detection, not software
capture calibration. Sensor setup is owned by [S3](subsystem3.md#22-local-controls).

Use `mqtt_client.py monitor` on the battery-powered normal robot with USB
disconnected, following the [power policy](../../README.md#power-and-flashing).
`WHEEL` JSON records contain robot `sampled_ms`, signed `target_left/right` and
`measured_left/right` in mm/s, signed `duty_left/right` percentages, `inhibited`
and `output_ok`. The dashboard shows these separately from averaged speed.
Duty is the serviced software command, not a measured electrical PWM or torque.
TEST always shows inhibition and zero serviced duty; it cannot tune loaded motors.
No new remote movement or gain-setting command exists.

1. Verify forward motor direction, encoder signs and rolling distance scale
   before gains. Preserve the recorded scale unless repeat measurements justify
   replacing it; check each wheel rather than hiding mismatch in an average.
2. Establish raised-wheel behavior and stop response, then assess loaded travel.
   Select intervals with stable equal wheel targets first, followed by stable
   unequal targets and low inner-wheel speeds. Changing targets are steering or
   ramp behavior; compare measured response against each wheel's actual target.
3. Establish static-friction feedforward `MOTION_KS` and speed feedforward
   `MOTION_KF` from measurements. Tune `MOTION_KP` for response and `MOTION_KI`
   for persistent error, one change per recorded trial; initially retain zero
   `MOTION_KD`. Shared gains currently serve both wheels. Do not raise the duty
   cap without the motor/driver ratings and electrical measurements.
4. If measured speed oscillates with constant targets, investigate wheel control,
   encoder noise, friction and power. If targets oscillate and wheels follow,
   investigate IR transitions/steering. Once wheel response is established,
   tune curve authority, steering slew and travel speed.

The [trace protocol](../architecture.md#wheel-trace-15-bytes) is best effort:
timestamp gaps expose missing releases and samples may drop offline or under TX
load. First/duplicate-time and stopped-path speed measurements are unavailable,
not measured zero. The nominal speed estimator is count displacement per Motion
release; quantization remains visible. Network arrival timing is not control time.
Timing/load/physical acceptance remains D07/D13; neither a graph nor a host test
establishes a physical pass.

## Status

Remote main `5e6a45b` includes command mutex, persistent stop, Motion health,
quadrature driver, motor driver, wheel PID, expiry, stall detection and discrete
actions. The pinned baseline used a separate Safety task. In the working implementation
Motion checks stop/fault/expiry itself and Mission calls bounded safety policy. Neither code nor host tests establishes
physical PID/angle/stop acceptance; see [S2 results](../../testing/subsystem2/TEST_LOG.md).

Remote encoder signs are +1/+1; the local checkpoint uses -1/+1. Nicholas has
not confirmed polarity for the final setup. Verify actual forward wheel direction
and count sign with raised wheels; never flip a sign solely to make source match.
Driver-selected LOW/LOW and acknowledged braking do not establish stopping time.

## Open items

Calibrate each motor polarity, encoder sign/counts, wheel/track geometry, speed
loop then line loop; verify distance and 90/180 degree turns, one wheel on a hump,
stall and GP21. Measure GPIO IRQ maximum edge load, sensor-to-output latency,
output-brake latency and physical stopping separately. Preserve host boundary/
expiry/action tests and run final hardware checks through MR-14..25/T02/T05/T15.

Stage #130 working implementation: GP21 is read before the command copy and
again inside the output critical section. Motion latches faults, expiry and
failed actions itself. The former Safety loop is a bounded `safety_step` called
by Mission; no Safety task is created. Its existing age limits remain provisional
with user authorization on 2026-10-29. GP21 release resets after cause clearance
and serviced braking; GP20 only starts from READY. A hung Motion/kernel remains
unprotected by these software checks. Physical latency remains unmeasured.

Integration correction: boot begins with one unserviced stop request, so no brake
acknowledgement exists before Motion's first successful brake. Wheel calculations
run with interrupts enabled; the final stop/GP21/expiry guard and motor write are
inside the output critical section. No physical latency is inferred from this.

Navigation revision (2026-10-31, INT-84): TURN's existing steering slot now
selects pivot (0) or forward single-wheel swing (1). Recovery and bypass retain
pivots. See the [Motion contract](../architecture.md#motion-command-contract);
Mission ends navigation from fresh turning-side IR edges, while Motion alone
writes/brakes outputs and retains command expiry/action bounds.

FOLLOW response revision (2026-10-31, INT-85): ramp the common base speed
before mixing steering, replacing independent mixed-wheel ramps. This
retains correction strength from the first release after a navigation brake.
See the [Motion contract](../architecture.md#motion-command-contract) and
[stored representation](../project-specs.md#pending-decisions). Speed-loop
and physical curve acceptance remain unmeasured.

Applied steering/pivot revision (2026-10-31, INT-92): Motion now smooths
steering as well as the common FOLLOW base before wheel mixing. Sustained
full correction brakes the inner wheel while the outer remains forward.
STOP/invalid/stale/zero-speed requests bypass both ramps. See the
[Motion contract](../architecture.md#motion-command-contract) for timing and
[stored representation](../project-specs.md#pending-decisions) for private Q7
history. Physical smoothness and pivot geometry remain unmeasured.

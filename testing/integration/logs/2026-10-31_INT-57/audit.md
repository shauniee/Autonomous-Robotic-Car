# Line tracking lifecycle audit — INT-57

## Contents

- [Scope and evidence](#scope-and-evidence)
- [Implemented lifecycle](#implemented-lifecycle)
- [Findings](#findings)
- [Smallest correction direction](#smallest-correction-direction)
- [Limits](#limits)

## Scope and evidence

Audited the current working tree, including existing uncommitted recovery
changes, on base `8981d04620d5537e4f5b68980528b452b752abb1`.
No application source or tuning changed in this audit. Exact input hashes:
[manifest](source-manifest.json). Procedure was defined before execution in
[case](case.json). Existing host suite passes:
[suite output](existing-host-suite.txt). Additional behavior assertions pass
in [PID](pid.txt) and [binary](binary.txt) profiles using [repro](repro.c).
These assertions reproduce the findings; they are not acceptance of the policy.

Read acquisition/classification, snapshot publication/freshness, Mission and
Safety, standalone recovery, Motion controller and wrapper, encoder arithmetic,
motor output, adjacent junction confirmation, owning guides and existing tests.
The reproduction runs actual Mission/recovery/Motion pure code. It models the
wrapper failure latch as Mission feedback, without running kernel tasks.

## Implemented lifecycle

| Stage | Actual software behavior |
| --- | --- |
| Acquire | Paired ADC acquisition every nominal 20 ms; failed pair is invalid. Normalization uses independent black/white references. |
| Classify | Both fractions below 0.15 enter LOST; prior LOST persists while both stay below 0.25. Both above 0.85 are BROAD_BLACK. Other valid pairs are VISIBLE. These are provisional thresholds. |
| Track | PID or binary guidance updates per distinct visible acquisition. Motion FOLLOW action 1 applies positive steering by speeding the left wheel and slowing the right. |
| Brief loss | Fresh LOST renews command freshness but holds the last steering and its calculated speed. It does not update line PID. Continuous loss must exceed 1000 ms to enter recovery. A visible acquisition clears the pending loss timer. |
| Enter recovery | Reset line controller history, issue STOP, wait for 150 ms without encoder count changes. Sweep origin is captured after stopping. |
| Search | Pivot toward +100°, then alternate toward -100°/+100°/-100°. Brake before reversal, wait for stationarity, allocate a new action ID for the next pivot. Relative turn goals stay fixed during an action. |
| Detect | A usable VISIBLE sample interrupts the pivot with STOP. Current Motion code returns IDLE for explicit STOP. BROAD_BLACK does not satisfy visible detection. |
| Confirm | While stopped, count three distinct stationary VISIBLE acquisitions. Moving samples reset the streak; LOST restarts search. There is no sensor-error or heading alignment criterion in standalone recovery. |
| Resume | DONE enters FOLLOW_LINE and resets PID; that release still commands STOP. The next release issues normal-speed FOLLOW with slew-limited steering. |
| Fail | Required stale/invalid sensing, stop, actuator failure, obstacle, or exhausted recovery bounds stops movement. Recovery has a 30 s deadline, four sweep legs, heading bound and net wheel-excursion bound. |

Acquisition timestamps survive the pipeline; cached samples cannot qualify
loss duration or confirmation indefinitely. Recovery TURN commands use the IR
acquisition timestamp. Motion independently checks expiry, local stop and faults.
Networking does not drive the motors.

## Findings

1. **High priority: visibility-only handoff immediately restores high speed.**
   `recovery_control.c:63–88` confirms visibility rather than alignment;
   `mission_logic.c:657–659` enters ordinary tracking. In both reproduced
   profiles, an encoder heading of 80° relative to the stopped search origin
   and sensor error +100 are accepted after stationary confirmation. The next
   command is 431 mm/s with steering +8/127. The steering slew starts at zero,
   and curve slowdown depends on this small commanded steering, so speed is
   close to the 450 mm/s nominal value despite large current sensor error.
   This follows the current documented stable-visible handoff, but leaves a
   rejoin weakness: stationary detection proves neither an aligned route nor
   sustained tracking while moving. The synthetic angle is not a measured
   angle relative to the physical line; the test proves acceptance and output,
   not a physical departure. Do not reinstate arbitrary alignment pivots solely
   from this result: two sensor fractions cannot establish route heading.

2. **High priority: immediate re-loss after recovery receives a fresh blind interval.**
   `mission_logic.c:380–394` treats post-recovery loss like any ordinary gap.
   The reproduction supplies fresh LOST immediately after the first FOLLOW:
   431 mm/s and +8/127 remain commanded through a full 1000 ms interval before
   another recovery starts. There is no moving rejoin confirmation or special
   handling for a recovery that has just failed to hold the line. At that
   commanded speed, one second corresponds to 431 mm of requested travel;
   actual travel is unmeasured. The one-second ordinary loss delay is an
   existing user-requested setting; this audit does not propose silently
   changing that setting for established tracking.

3. **Medium priority: PID history survives a long interruption in visible guidance.**
   `mission_logic.c:110–152` detects a discontinuity and skips integration,
   but retains the old integral and held steering. With +64 error for 3 s,
   then 800 ms of fresh LOST, the next visible error -64 yields steering
   **+68/127**. The integral remains 13107/32767 (0.40). Thus the controller
   carries old bias through the loss and initially corrects in the wrong
   direction. This is the short-loss path, not the full recovery path, which
   already resets PID. Binary mode resets its buildup and returns -8/127 in
   the same fixture. Some transient lag from steering slew is deliberate;
   retained integral after discontinuity is an additional independent bias.

4. **Adjacent known issue: outgoing-junction confirmation bypasses ordinary loss recovery.**
   `mission_logic.c:593–606` commands straight FOLLOW at 90 mm/s during LOST
   and ignores nonzero visible error until confirmation completes. Its bound
   is the 8 s navigation deadline, not the 1 s loss interval. Prior INT-56
   already reproduces this behavior. If telemetry says NAVIGATE rather than
   REACQUIRE, this is a different path and remains unresolved.

The earlier STOP-to-FAILED defect is corrected in the current working tree:
explicit recovery braking returns IDLE; absent/expired active actions still
fail. The existing tests and this reproduction verify the pure controller
feedback path. The prior INT-55 source/build comparison is recorded in
[comparison](prior-build-comparison.json); it cannot identify what is flashed.

## Smallest correction direction

- Preserve the bounded sweep and intentional STOP-to-IDLE behavior.
- Add a bounded, reduced-speed moving rejoin before accepting established
  tracking; early loss during that rejoin should return to search or stop.
  Choose and document its speed/bounds without inventing calibrated values.
- Clear stale PID integral/history when visible guidance resumes after a
  discontinuity, with an explicit decision about preserving steering slew.
- Handle junction outgoing confirmation separately; the standalone recovery
  changes do not reach that path.

These are proposed changes, not implemented or physically validated behavior.

## Limits

No firmware build, flash, powered test, physical geometry model, wheel polarity
verification or running-device identity check. Encoder pose, stationarity and
IR are synthetic. Two analog sensors cannot prove which route was detected.
The findings establish source behavior that can explain failed rejoining, not
the confirmed cause of a particular unrecorded physical run.

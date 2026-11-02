# Straight NAVIGATE slowdown and jitter audit — INT-59

## Contents

- [Answer](#answer)
- [Reproduced behavior](#reproduced-behavior)
- [Correction scope](#correction-scope)
- [Evidence and limits](#evidence-and-limits)

## Answer

The user clarified this happens when crossing straight without a turn.
That path has not been fixed. Current application sources and the locally
built UF2 match INT-58, whose change addressed recovery sweeping. The image
on the robot is not verified. No application code or tuning changed in this audit.

## Reproduced behavior

1. **Slow movement is explicitly requested.** `mission_logic.c:606` uses
   `MISSION_BROAD_SPEED_MMPS`, currently 90 mm/s, throughout NAVIGATE/CONFIRM.
   Ordinary straight tracking uses 450 mm/s. The software requests one fifth
   of nominal tracking speed here; these are commands, not measured speeds.
   The slower broad-black crossing itself is an existing provisional design,
   not proof of a broken motor controller. Confirmation can prolong it.

2. **Straight transitions contain STOP pulses.** `mission_step` defaults the
   output command to STOP (`mission_logic.c:674`). Both the straight encounter
   entry (`:351–357`) and the successful confirmation exit (`:595–599`) change
   state and return without replacing that command. The fixture reproduces:
   broad crossing at 90 -> STOP on entry -> 90 during confirmation -> STOP on
   exit -> 450 on the next release. Each STOP lasts one nominal Mission release
   (10 ms); Motion releases nominally every 20 ms, so whether it consumes an
   individual pulse depends on scheduling. If consumed, Motion brakes and
   resets its action and wheel PID. Repeated qualified encounters reproduce
   repeated boundary brake/restart requests. This is a source-level jerk
   mechanism, not a measurement of the user's physical vibration. A steady
   NAVIGATE/CONFIRM interval does not emit these pulses each release.

3. **Confirmation can keep driving slowly with no line correction.** Fresh
   LOST commands 90 mm/s, zero steering, action 1 for the reproduced 2 s.
   It does not enter ordinary one-second line-loss recovery. The navigation
   deadline is 8 s from entry. Visible readings with error +100 still receive
   zero steering until the third distinct visible acquisition completes
   confirmation. Missing that outgoing line can prolong the slow phase.

4. **No action-ID churn or turn/distance hunting on this route.** While
   confirming, the command stays FOLLOW/action 1. The post-turn clearance
   DISTANCE action and TURN endpoint behavior are not executed. Their known
   behavior cannot explain the clarified straight route.

5. **Nominal Motion model does not reproduce steady-state stop/start.** The
   existing quantized idealized plant at a fresh constant 90 mm/s FOLLOW
   command has duty 0.066364..0.098499 over its final 2 s, with zero braked or
   reverse samples. That model is linear and lacks measured friction,
   inertia, supply behavior, encoder electrical faults and scheduler timing.
   This result cannot rule out physical low-speed stick/slip or wheel-loop
   oscillation. The current tests do not establish calibrated operation at
   this speed. Do not change encoder signs or PID gains from this model alone.

## Correction scope

The smallest source correction should keep a valid continuous FOLLOW command
across the two straight transitions, apply visible-line guidance during
outgoing confirmation, and bound LOST confirmation rather than continuing
blindly to the full navigation deadline. Retain reduced speed while genuinely
crossing broad black; choose any revised confirmation speed as an explicit
trial setting. Neither arbitrary speed increases nor DISTANCE-only changes
resolve all findings above. These changes are proposed, not implemented.

If the robot jitters throughout one unchanged NAVIGATE/CONFIRM interval,
physical wheel counts and applied duties at the Motion release rate are still
needed to distinguish low-speed control/friction from transition pulses.
Existing dashboard wheel speeds are derived from slower telemetry count
differences; they cannot establish per-release drive/brake timing. Current
telemetry/state messages also do not identify the navigation subphase.

## Evidence and limits

Procedure: [case](case.json). Exact working-tree sources and base commit:
[manifest](source-manifest.json). Source/image comparison:
[comparison](build-comparison.json). Reproduction:
[source](repro.c), [output](repro-output.txt). Existing pure suite:
[output](host.txt), PASS including default/guarded Motion and PID/binary Mission.
New assertions PASS by reproducing the behavior; this is not acceptance of it.

No firmware edits, new build, flash or physical run. Encounters and IR are
scripted; the nominal wheel plant is synthetic. No claim of measured jitter
frequency, actual stopping response, safe minimum motor speed or flashed-image
identity is made. INT-56's distance-drift finding is explicitly excluded from
the user's clarified straight-crossing symptom.

# Subsystem 5: Obstacle sensing and profiling

## Contents

- [Functionality](#functionality)
- [Interfaces and contract](#interfaces-and-contract)
  - [Profiling method to implement and validate](#profiling-method-to-implement-and-validate)
  - [Bypass handover and failure exits](#bypass-handover-and-failure-exits)
- [Data and fault bits](#data-and-fault-bits)
- [Status](#status)
- [Open items](#open-items)

## Functionality

Owner Jia Jing. [Plan](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117) owns sequence;
[architecture](../architecture.md#mission-loop) owns bypass/stop authority;
[team](../team.md#2-pins) owns wiring. Target S5 is bounded sonar/servo acquisition
and profiling called by Mission on core 1, not an independent task. Mission
chooses movements; only Motion drives motors.

Professor clarification: estimate and report obstacle widths 100-200 mm; accuracy
matters but no numerical tolerance was supplied. Evaluate safe path selection,
full-car clearance, original-line recovery and reasonable speed as well as width
error. Straight-only obstacle placement is an operating assumption, not proof
that the general course excludes curves. Sonar electrical behavior/wiring are
user-confirmed; measured timing/geometry/uncertainty still need evidence.

## Interfaces and contract

Current APIs `s5_range_samples`, `s5_range_try_copy`, `s5_scan_request`, `s5_scan_cancel`,
`s5_scan_word`, `s5_side_watch`, `s5_profile_snapshot` and `s5_profile_try_copy`
are reusable boundaries.
Advance trigger/Echo/servo-settle/scan phases by timestamps and short capture ISRs;
never busy-wait through Echo or servo movement. Full/cancel/deadline outcomes are
explicit. Commanded servo angle is not a measured angle.

Forward and side evidence have separate validity/acquisition times. Side scans
never refresh forward coverage. Ordinary no echo is unknown/report-only, neither
infinite clearance nor a universal stop. A manoeuvre without its required clearance
evidence cannot proceed merely because ordinary tracking tolerates missing range.

### Profiling method to implement and validate

- Stop and confirm measured stationarity; settle at each calibrated angle.
- Take a bounded group of samples, median-filter valid ranges and retain invalid
  counts/status. Choose sample counts and intervals from physical testing.
- Identify obstacle regions across consecutive angles, with hysteresis and
  background-distance comparison where a usable background exists.
- Estimate span/closest point/clearance using range, angle, mounting geometry and
  uncertainty. Report the estimate, not the maximum expected width as a measurement.
- Plan conservatively with the 200 mm upper bound where useful, full car envelope
  and calibrated margins. Unknown edge/clearance is not a supported gap.
- Use a second stationary viewpoint only when movement there is supported; it is
  an optional refinement after a simple measured scan/bypass works.

### Bypass handover and failure exits

Mission sequences departure, parallel side-following, rear-edge confirmation,
rear-of-car clearance, inward approach and original-line alignment. A single
lost echo or range jump is not enough to prove pass-by. Encoder travel and
repeated observations support depth and clearance decisions. Schedule forward
checks because one servo-mounted sensor cannot watch side and front simultaneously.

The requested left 50-degree scan/45-degree departure are candidate geometry,
not universal calibrated limits; scan both sides and select a supported route.
Reversing is permitted only with a bounded supported path/clearance. Do not infer
rear clearance from forward sonar. Both-sides-blocked, failed profile, insufficient
coverage, new hazard and timeout brake/stop explicitly; rescan/retry policy is
pending D02/D05/D07. No added generic recovery framework.

## Data and fault bits

[Specs](../project-specs.md#5-data-representation-table-week-13) owns scan/range/
profile widths and invalid codes; [architecture](../architecture.md#fault-bitmask)
owns stop/start policy. Preserve binary schema v2 width/depth fields and events.
Estimation uncertainty may be recorded in bounded diagnostic capture/test evidence
without changing the wire schema. Never invent a precision rating.

## Status

Remote `5e6a45b` contains Echo capture, PWM servo, scan/profile builder, side-watch,
packed range/scan/profile values and a separate Obstacle task. Physical quality,
median/hysteresis improvements, supported-side selection and original-line bypass
must be verified rather than inferred from code. [S5 results](../../testing/subsystem5/TEST_LOG.md)
retain their exact source/results. Working `s5_step` now advances trigger, Echo, settle and coarse/fine acquisition
on Mission releases without waiting through a scan. Motion/cancel/deadline guards
apply each step; front/side words are separate. Median groups, adjacent-ray edge hysteresis, edge-focused fine angles and optional
empty-scene comparison now have bounded implementations and synthetic tests.
Model uncertainty is distinct from planning width; unknown uncertainty provides
no supported planning gap. Operating sample/threshold settings are configured
as provisional proposals in [project specs](../project-specs.md#pending-decisions). `MISSION_BYPASS_RELEASED=0` prevents an unaccepted envelope/coverage
policy from authorizing movement. Forward/side coverage and rear-edge/original-line
acceptance, including implementation corrections found during trials, remain
tracked in [#117](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117).
The bypass helper uses qualified repeated side evidence; lost Echo never proves
a rear edge. Its geometry and physical performance are still unaccepted.

Servo commands now slew by one degree no faster than every `SERVO_STEP_MS`
(proposed 50 ms), including recentring and side-watch moves. Acquisition waits
until the target command is reached plus `SERVO_SETTLE_MS`; delayed releases
advance only one degree. The scan deadline is 20 seconds to accommodate this
slower movement. These are software settings, not measured servo performance;
D05 physical calibration remains pending. Scans now visit every 5 degrees from
-20 through +20 (nine positions). Each position holds for settling and the
configured median sample group before moving onward. Settled groups publish
immediately to the range snapshots: only the 0-degree group refreshes forward
coverage; other angles update side evidence. The nine positions fill scan
storage, so no additional fine rays are scheduled with this configuration.

## Open items

D02/D03/D05/D07/D12/D16: servo calibration, repeat/settle times, sample/filter/edge
parameters, car envelope, reverse/scan/travel budgets and 15 cm interpretation.
Use MR-17/18/30/31 and T08..T10/T14/T15: reference widths across 100-200 mm,
backgrounds/materials/angles, both sides blocked, lost Echo, moving scan rejection,
forward refresh, uncertainty, depth, clearance and repeated line reacquisition.

The confirmed trigger meaning is in D16. Mission and Reporting now age sonar
using full 32-bit acquisition times; the 16-bit packed time is display-only.
Mission reads its own acquisition records, while Reporting uses a nonwaiting
coherent mirror under the existing profile mutex. Busy publication retains the
original sample time and cannot revive a frozen sensor. The profile snapshot
coherently includes model evidence and scan completion time; MQTT sizes stay
unchanged. See [continuation audit](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117#done-when).

# Obstacle

## Contents

- [Scope](#scope)
- [Files](#files)
- [Lecturer expectations](#lecturer-expectations)
- [Contract](#contract)
- [Scan and bypass](#scan-and-bypass)
- [Curves near an obstacle](#curves-near-an-obstacle)
- [Physical checklist](#physical-checklist)
- [Status and open items](#status-and-open-items)

## Scope

Owner: Jia Jing. Bounded sonar and servo acquisition with uncertain width and clearance estimates, called by Mission on core 1. Mission chooses movements and only Motion drives motors. Safe navigation is the primary success criterion. Authority: [Mission loop](../../docs/architecture.md#mission-loop). Wiring: [team](../../docs/team.md#2-pins).

## Files

| File | Job |
| --- | --- |
| `obstacle.h` | The subsystem's only header |
| `obstacle.c` | `obstacle_step`: trigger, Echo, servo settle, scan phases, range snapshots |
| `obstacle_profile.c` | Range filter, edge detection and profile (host-tested) |

## Lecturer expectations

Confirmed on 2026-10-25, with the original rubric not reviewed:

- Keep the prescribed HC-SR04 and swivelling servo; handle obstacles 10-20 cm wide.
- Estimate span and surrounding clearance reliably enough for safe navigation, and report uncertainty explicitly.
- Show consistent safe path selection, repeated clearance and line reacquisition.
- Exact width measurement is not required. Using 20 cm as a conservative planning width is allowed, but it cannot establish unknown clearance.

Suggested, not mandatory (record any omission and why): servo settling, repeated measurements, median filtering, background subtraction, consecutive-angle detection, hysteresis, conservative margins, multiple robot positions. Choose values by calibration; no numeric thresholds were supplied.

## Contract

- Current APIs: `obstacle_range_samples`, `obstacle_range_try_copy`, `obstacle_scan_request`, `obstacle_scan_cancel`, `obstacle_scan_word`, `obstacle_side_watch`, `obstacle_profile_snapshot`, `obstacle_profile_try_copy`.
- Phases advance by timestamps and short capture ISRs, never a busy-wait. Commanded servo angle is not a measured angle.
- Front and side evidence have separate validity and times; side scans never refresh forward coverage.
- No echo is unknown and report-only: neither infinite clearance nor a universal stop. A manoeuvre starts only from a supported profile.
- Sonar age uses full 32-bit acquisition times; the 16-bit packed time is display-only.

Widths: [data table](../../docs/team.md#6-data-representation). Stop and start policy: [fault bitmask](../../docs/architecture.md#fault-bitmask).

## Scan and bypass

Nicholas, 2026-11-05: the obstacle only needs clearance to bypass, so depth is not measured or reported. The side look only times the pivot back. Telemetry and the obstacle event report the side chosen and its clearance.

- **Scan:** stop and confirm stationary, then visit every 10 degrees from -60 to +60, plus one 5 degree ray at each edge. Servo commands slew one degree per `SERVO_STEP_MS`; each position settles, takes a median sample group, and publishes it. Only 0 degrees refreshes forward coverage. A ray with no echo gets one more group before it counts as no echo. A ray belongs to the box if it lands within 40 mm behind the front face. No echo past the box marks its edge, but only a real echo counts as clearance; a lost ray with the box on its far side stays part of the box. The deadline is 20 s. When a sweep ends, with or without a profile, the servo returns forward and Mission reads ahead before using the result: three clear readings in a row (nothing within 150 mm, or no echo) mean the obstacle was removed, so the car carries on following the line; one blocked reading continues as below. A sweep with no profile retries once, then stops. A scan that shows the car can pass is swept once more and the narrower width kept, because the beam catches the box corner on some sweeps; telemetry reports that width.
- **Estimate:** identify obstacle regions across consecutive angles with hysteresis, then estimate span and closest point. Clearance is the sideways reach of the free rays (`range * sin(angle)`, smallest wins) minus the obstacle's planned edge. Report the estimate, never the maximum expected width as a measurement. Echoes just past an edge at 1.75 to 2.25 times the edge ray's range are ignored as ghosts, sound bouncing box, car, box (INT-201), and at least two real echoes must lie beyond them. Unknown edge or clearance is not a supported gap.
- **Bypass:** Mission works out each edge's sideways distance from the servo axis (`forward range * tan(edge angle)`) and takes the shorter one; a tie goes left. Clearance does not affect the choice (Nicholas, 2026-11-05). It pivots 90 degrees toward it and drives the distance from the centre to that edge plus half the car's length (125 mm, nose to rear without the barcode arm), raised to 135 mm when the barcode arm side faces the box. That leaves the passing side 50 mm off the measured edge: the 10 mm too-close limit plus a 40 mm allowance for scan and heading error. It pivots 90 degrees back, looks 60 degrees toward the box and drives on without stopping. Readings before the ray reaches the front of the box are ignored, since the car starts level with where it stopped. Once the ray is off the box for 60 mm of travel, it works out the rear position, drives until the facing side will clear it by 20 mm (10 mm let the rear wire hit), pivots 90 degrees toward the line and drives until a sensor sees it, then advances and pivots as at a junction. Each leg is one continuous drive. The rear comes from the side range the ray last measured, but the car always passes at least 200 mm of box (`BYPASS_MIN_DEPTH_MM`, provisional), because a flat side can return no echo or only its front corner. Telemetry reports what the ray saw, so a depth well under the real box means the ray left early. If the car is too close to pivot, Mission reverses far enough to pivot and scans again. If the leg out would pass `BYPASS_MAX_OFFSET_MM`, it reverses 50 mm and scans again. Reversing is blind along the approach and totals at most 150 mm for one obstacle, then it stops. No echo on a bounded leg is not a hazard; only a close echo or a silent sensor stops it. If three side ranges in a row are shorter than the car's side plus 10 mm, the car brakes, pivots 90 degrees away, drives 40 mm further out, pivots back and passes again. `BYPASS_MAX_OFFSET_MM` limits how far, and if the box is already within the rear corner's swing it stops instead. There is no forward check while looking sideways. Failed profile, too wide or still too close after the full reverse, new hazard, no line on the return or timeout brake and stop.
- **Release:** `MISSION_BYPASS_RELEASED` is 0 in source, and the `./flash.sh` full-course image sets it to 1. Bypass geometry and physical performance are still unaccepted, so supervise those runs.

## Curves near an obstacle

Options only; none is built. Obstacles on straights are the operating assumption (D12).

The bypass plans in the car's frame at the stop, and it treats the line as straight through that point:

| Case | What happens with the current code |
| --- | --- |
| Curve before the box | The car stops at an angle to the line, so width, clearance and the return target are skewed. The sonar faces along the car, so a box on the bend can be seen late. An object beside the line can trigger a scan |
| Line after the box bends toward the pass side | The line can be crossed during `PASS`, where it is not checked. The car then fails or stops |
| Line after the box bends away from the pass side | `RETURN` drives back to the old line position, finds no line and stops |
| Rejoin direction | After the touch, Mission pivots by a fixed junction turn (`m->bypass.left_side`), which assumes the line is square to the car |

These cases apply to bends in either direction and to either pass side. All of them end in a bounded stop, not a blind drive, but host tests do not cover curves and no car run has tested them.

| Option | Change | Limit |
| --- | --- | --- |
| A. Refuse on a bend | Stop safely when `line_i_q` shows a sustained touch at the trigger | Scores no bypass on a bend, but the behaviour is honest and predictable |
| B. Heading at the stop | Estimate entry heading from encoder travel since the last centred tracking (a new stored origin, since none exists now), and plan in the line's frame | Encoder drift and slip; no gyroscope. Fixes a curve before the box only |
| C. Line search after the box | Check the line during `PASS` and search with `recovery_control` instead of one fixed return | Could join a branch or the wrong direction (A9); needs heading limits |
| D. Rejoin pivot from sensors | Pivot until the sensors straddle the line, with the turn limited to the forward direction | Needs a measured tightest curve radius |
| E. All of B, C and D | Full curved bypass, either direction | Largest change and the most car time |

Without A, a bend ends in a bypass failure or a false scan. If curves become required, build A first, then C and D, then B. Measure the tightest curve radius before any of these.

## Physical checklist

No situation below has a recorded physical pass; host tests use synthetic data. Run each repeatedly, then record date, revision, setup, trial count, estimate, uncertainty, decision and outcome in the [obstacle test log](../../testing/obstacle/TEST_LOG.md). Do not invent a pass rate.

| Situation | Expected behaviour |
| --- | --- |
| Clear track | Keep following and reporting front range; no bypass |
| Obstacle blocks the line | Stop safely, wait until stationary, scan, then form a profile or fail explicitly |
| 10, 15 and 20 cm wide | Report an estimate; pass on the side with the shorter distance to the edge |
| Something beside the box | Not checked before choosing; only the too-close and forward checks can stop the car |
| Leg out past the offset limit | No bypass; reverse and rescan within the limit, then stop |
| No, intermittent or stale echo | Unknown, never proof of clear; authorizes nothing |
| Noisy scan or scan timeout | No confident profile; retry only where policy permits, else stop |
| Moved or cancelled during a scan | Reject the result; leave the sensor in a predictable state |
| Different approach distances | Keep decisions defensible; reject profiles lacking edge or clearance evidence |
| Bypass underway | Keep observing; do not declare clear from the initial scan |
| Rear edge detected | Return, reacquire the line, resume following; depth is not reported |

## Status and open items

Echo capture, median groups, edge hysteresis, profile and clearance estimation and scan guards are host-tested only.

- D02, D03, D05, D07, D12 and D16: servo calibration, timing, sample and edge parameters, car envelope and travel budgets.
- Measure sonar and servo behaviour, then run both-blocked and failure cases before both-side bypass runs (MR-17, MR-18, MR-30, MR-31, T08-T10, T14, T15).
- Forward coverage during bypass and rear-edge handling still need physical evidence. Compare this list against the original rubric.

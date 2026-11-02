# Obstacle (S5)

## Contents

- [Scope](#scope)
- [Files](#files)
- [Lecturer expectations](#lecturer-expectations)
- [Contract](#contract)
- [Scan and bypass](#scan-and-bypass)
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
| `obstacle_test.c` | Firmware self-tests |

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
- No echo is unknown and report-only: neither infinite clearance nor a universal stop. A manoeuvre without its required evidence does not proceed.
- Sonar age uses full 32-bit acquisition times; the 16-bit packed time is display-only.

Widths: [data table](../../docs/project-specs.md#5-data-representation-table-week-13). Stop and start policy: [fault bitmask](../../docs/architecture.md#fault-bitmask).

## Scan and bypass

- **Scan:** stop and confirm stationary, then visit every 10 degrees from -50 to +50, plus one 5 degree ray at each edge. Servo commands slew one degree per `SERVO_STEP_MS`; each position settles, takes a median sample group, and publishes it. Only 0 degrees refreshes forward coverage. The deadline is 20 s.
- **Estimate:** identify obstacle regions across consecutive angles with hysteresis, then estimate span and closest point. Clearance is the sideways reach of the free rays (`range * sin(angle)`, smallest wins) minus the obstacle's planned edge. Report the estimate, never the maximum expected width as a measurement. Unknown edge or clearance is not a supported gap.
- **Bypass:** Mission sequences departure, passing with the servo 50 degrees toward the obstacle, rear-edge confirmation, rear clearance and return. The first black sensor on the return ends it, then Mission advances and pivots as at a junction. Depth comes from the encoder position at the first rear-edge reading, so it can read long by up to a 50 mm step. One lost echo never proves a rear edge. Both sides blocked, failed profile, poor coverage, new hazard, no line on the return or timeout brake and stop.
- **Release:** `MISSION_BYPASS_RELEASED` is 0 in source, and the `./flash.sh` full-course image sets it to 1. Bypass geometry and physical performance are still unaccepted, so supervise those runs.

## Physical checklist

No situation below has a recorded physical pass; host tests use synthetic data. Run each repeatedly, then record date, revision, setup, trial count, estimate, uncertainty, decision and outcome in the [obstacle test log](../../testing/obstacle/TEST_LOG.md). Do not invent a pass rate.

| Situation | Expected behaviour |
| --- | --- |
| Clear track | Keep following and reporting front range; no bypass |
| Obstacle blocks the line | Stop safely, wait until stationary, scan, then form a profile or fail explicitly |
| 10, 15 and 20 cm wide | Report an estimate; choose only a path with measured clearance for the whole car plus margin |
| One side blocked | Reject that side; bypass the other if clearance is enough |
| Both sides blocked or too narrow | No bypass; stop or report blocked |
| No, intermittent or stale echo | Unknown, never proof of clear; authorizes nothing |
| Noisy scan or scan timeout | No confident profile; retry only where policy permits, else stop |
| Moved or cancelled during a scan | Reject the result; leave the sensor in a predictable state |
| Different approach distances | Keep decisions defensible; reject profiles lacking edge or clearance evidence |
| Bypass underway | Keep observing; do not declare clear from the initial scan |
| Rear edge detected | Record depth, return, reacquire the line, resume following |

## Status and open items

Echo capture, median groups, edge hysteresis, profile and clearance estimation and scan guards are host-tested only.

- D02, D03, D05, D07, D12 and D16: servo calibration, timing, sample and edge parameters, car envelope and travel budgets.
- Measure sonar and servo behaviour, then run both-blocked and failure cases before both-side bypass runs (MR-17, MR-18, MR-30, MR-31, T08-T10, T14, T15).
- Forward coverage during bypass and rear-edge handling still need physical evidence. Compare this list against the original rubric.

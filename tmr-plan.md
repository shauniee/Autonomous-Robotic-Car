# Plan for tomorrow

## Contents

- [Where things stand](#where-things-stand)
- [1. Hump: confirm the fix on the car](#1-hump-confirm-the-fix-on-the-car)
- [2. Line tracking: measure it, then reach 95%](#2-line-tracking-measure-it-then-reach-95)
- [3. Barcode: stress test](#3-barcode-stress-test)
- [4. Obstacle: bench first, then bypass](#4-obstacle-bench-first-then-bypass)
- [5. Faults seen and not explained](#5-faults-seen-and-not-explained)
- [6. Housekeeping](#6-housekeeping)

## Where things stand

| Area | State | Evidence |
| --- | --- | --- |
| Line tracking | Good laps seen, but the last run went off course and was not smooth. No measured success rate. | User observation, INT-158; cause not analysed |
| Barcode | User reports fully functional. All letters and a flipped print not tested. | User observation, INT-156 |
| Hump | Latest fix built, never run on the car. Last run counted 5 for 1 crossing. | INT-158, host and build only |
| Obstacle | Host-tested only. Servo and sonar bench not run. | S5-U09, S5-U10 pending |

Order: the hump is first because the image is already built and it takes one run. Tracking comes next because every later test depends on it.

## 1. Hump: confirm the fix on the car

Flash the current image, let the car sit still on the track for a second, then press GP20.

- [ ] Cross the 10 mm hump at least 3 times with barcode turns in the lap. Pass: `humps=` matches the crossings exactly, and pivots add nothing (INT-158 bug).
- [ ] Note the reported heights against 10 mm. `HUMP_CALIB_FACTOR` 0.8 comes from one hump height.
- [ ] Repeat with a taller hump, up to the 30 mm limit, to check the scale (MR-29).
- [ ] Hump on a curve, and a hump just before a barcode.
- [ ] Turn accuracy with one wheel on a hump (MR-23).
- [ ] Reverse direction over a hump: the side and roll sign are untested.
- [ ] Revisit `HUMP_MIN_TRAVEL_MM` 50: it rests on only two crossings.

## 2. Line tracking: measure it, then reach 95%

The 95% target is the user's. No success rate has been measured yet, so the first job is to get one.

- [ ] Define one attempt and what counts as a success (for example, a full lap with no line loss), and write that down before running.
- [ ] Run a fixed number of laps with one image and no changes, and record the rate (MR-26). 20 laps gives a usable 95% check.
- [ ] Analyse the off-course run in `testing/integration/logs/2026-11-02_raw-captures/int-157-run.txt.gz`, near the end of the run.
- [ ] Check whether the diagnostic traces affect tracking: compare the same laps with them on and off. No evidence either way yet.
- [ ] Work through the [open issues](app_program/line_barcode/README.md#open-issues): junction on three curves, `NAV_ONSET_SKEW_MM`, pivot hand-over, real line loss recovery.
- [ ] Change one value per trial and record each as a new INT entry.

## 3. Barcode: stress test

- [ ] Print all 26 letters and run each at course speed, in both directions (MR-27). Host tests cover the patterns only.
- [ ] Flipped print, reading it from the other direction. C and P can complete early with an unknown orientation; nothing reads orientation today.
- [ ] Inverted print (white bars on black), if the course can have one. Decoding now needs black high (`BARCODE_BLACK_HIGH`), so an inverted print **will not read**. Decide whether this is needed.
- [ ] Action at the junction for each of left, right, straight and U-turn (MR-28).
- [ ] Record reads against passes as a rate. A corrupted last guard still loses the read (INT-155, third pass).

## 4. Obstacle: bench first, then bypass

Scan and bypass logic exists but has never run on the car. Wheels stay off for the bench steps.

- [ ] S5-U10 with `./flash.sh test`: servo centre and limits, with +/-60 degrees unconfirmed (MR-18).
- [ ] Sonar against a board at 100, 250, 500 and 1000 mm, plus open space (MR-17).
- [ ] Boxes of 100, 150 and 200 mm: check the `[SCAN]` rays and profile against a tape.
- [ ] Both sides blocked: the robot stops rather than bypasses (MR-31).
- [ ] Then bypass runs, one side open and both open, and line reacquisition (MR-30).
- [ ] Measure bypass speeds, settle time and travel budgets (D05, D07, D12, D16).
- [ ] Explain `SCAN` then `STOPPED` with `front=no echo` on broad black (INT-156 capture, from about line 8020).

## 5. Faults seen and not explained

- [ ] `Obstacle: no echo` appears in many runs. It is meant to be report-only; check it never causes a stop on its own.
- [ ] `barcode capture overflow` is frequent. It no longer clears the runs; consider a larger ring if reads still fail.
- [ ] Encoder faults stopped runs earlier, and the bad edges have no known cause. Telemetry also showed speed spikes such as `L=1935 R=-222` (INT-157 capture): check whether these are real or lost frames.
- [ ] `IMU calibration failed` on one boot (INT-154 capture).
- [ ] Pico restarts seen during earlier runs: confirm whether they were manual.
- [ ] `EVENT BARCODE` missing for one read that telemetry shows (INT-154 capture).

## 6. Housekeeping

- [ ] `scripts/dashboard.html` and `testing/pure/test_dashboard.mjs`: uncommitted local edits remove the robot animation and motion previews. Their origin is unconfirmed, so they were **left out of the merge**. Keep and commit, or discard.
- [ ] Close PR 147 (`chore/common-cleanup`), whose commit is included in this merge.
- [ ] Append the merge commit to the test log for INT-132 to INT-158, which ran on an uncommitted tree.
- [ ] Remove or agree the diagnostic frames (types 9, 10 and 11): Comms owner agreement is pending.
- [ ] Trim overlapping logs, for example INT-135 and INT-136, at 8.5 MB and 14 MB.
- [ ] Update the mission-readiness checklist as items pass.
- [ ] Delete this file once it is done.

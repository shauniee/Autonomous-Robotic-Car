# S5 test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)
- [2026-10-27 — Minimum-bit obstacle packing candidate (#111)](#2026-10-27--minimum-bit-obstacle-packing-candidate-111)
- [Documentation alignment — 2026-10-29](#documentation-alignment--2026-10-29)
- [Current helper coverage — INT-25](#current-helper-coverage--int-25)
- [Servo travel limit — S5-U05](#servo-travel-limit--s5-u05)
- [Slow servo scan — S5-U06](#slow-servo-scan--s5-u06)
- [Five-degree scan readings — S5-U07](#five-degree-scan-readings--s5-u07)
- [Wider scan and line-ended return — S5-U09](#wider-scan-and-line-ended-return--s5-u09)
- [Servo and sonar bench — S5-U10](#servo-and-sonar-bench--s5-u10)

Owner: Jia Jing / obstacle. Inactive placeholder. Retired template definitions: S5-U01 task-context example; S5-U02 unfilled manual placeholder. No template result was recorded. No subsystem hardware test is claimed.

## Test cases

Project feature tests: PENDING until the subsystem is implemented.

## Results

No project results recorded.

## 2026-10-27 — Minimum-bit obstacle packing candidate (#111)

| Date | ID | Source | Tester | Result | Evidence | Notes |
|---|---|---|---|---|---|---|
| 2026-10-27 | S5 CR-111 ray/profile/side-watch host regression + 4 profile builds | `b438bcc9` | GitHub Actions | PASS | [Workflow run 40518950503](https://github.com/shauniee/Autonomous-Robotic-Car/actions/runs/40518950503) | Packed fields round-trip signed angles -60..60 and full 16-bit range; simulated obstacle geometry; NOT physical servo/echo tests |
| 2026-10-27 | S5 CR-111 Cortex-M0+ sizeof probe | `b438bcc9` | arm-none-eabi-gcc (Docker build image) | PASS | Same workflow, Obstacle sizeof step | `ObstacleRay` 3 bytes, `ObstacleProfile` 14 bytes; side-watch holds 8 meaningful bits in one 32-bit atomic word |

Sonar measurement range, servo mechanics and D05 calibration must be tested on the robot before the full avoidance mission can be accepted.

## Documentation alignment — 2026-10-29

Planning note only; no test was executed and no historical result is changed.
The [central plan](../../plan.md) owns future implementation sequence;
[mission-readiness](../mission-readiness.md) owns target physical procedures.
Earlier task splits, GP20 reset flows, JSON messages, watchdog assumptions and
configured timing targets describe their recorded revisions. Do not interpret
them as target passes. Use the applicable procedure revision, exact source and
new evidence filenames for reruns; never overwrite evidence or reuse retired IDs.

New cases for the full-plan continuation (defined before accepted runs):

- S5-U03: kernel-free range groups; invalid/no Echo, isolated outliers, minimum-valid count, completion and representable boundaries. Expected: unknown groups never become clearance and repeated queries never add progress.
- S5-U04: qualified stationary profiles with synthetic 100/150/200 mm faces, adjacent rays, edge hysteresis, optional empty-scene background, conservative planning distinct from measurement, unknown uncertainty and duplicate/invalid rays. Expected: model-consistent estimates or explicit rejection; no physical accuracy claim.

| Date | ID | Source | Result | Evidence / limits |
| --- | --- | --- | --- | --- |
| 2026-10-29 | S5-U03 / S5-U04 | Base `044889041aac67b78742f3b7004cd545755349fb` plus [working-source manifest](../integration/logs/2026-10-29_INT-16-manifest.json) | PASS (host only) | [Host output](../integration/logs/2026-10-29_INT-16-host.txt); Bounded median/edge/model helpers and invalid/no-echo cases; synthetic fixture settings, not enabled production calibration |

## Current helper coverage — INT-25

2026-10-30: removed unused `s5_profile_build` / `s5_fine_angles` and their
surrogate host cases. Historical results above remain source-pinned. The
current `test_obstacle_profile.c` covers scan state/ID/guard and packed ray /
side-watch representations; `test_profile_edges.c` covers the estimator and
fine-angle selection used by firmware. All host checks and four firmware builds
pass at the [INT-25 manifest](../integration/logs/2026-10-30_INT-25-accepted-manifest.json);
[host output](../integration/logs/2026-10-30_INT-25-accepted-host.txt). No target
servo/echo, accuracy or physical bypass acceptance follows from this cleanup.

## Servo travel limit — S5-U05

Procedure defined 2026-10-30: run `testing/pure/run.sh` on the host with the
requested +/-20 degree configuration. Expect packed-ray boundary rejection and
side-watch clamping, synthetic profile/edge fixtures within the reduced sweep,
and Mission bypass geometry checks to pass. Synthetic faces use a 400 mm plane
to expose both edges within the smaller field of view; this is a fixture, not
a physical range or coverage acceptance. No hardware, firmware execution, wiring
or power is involved; D05 centre/scale/settle calibration remains pending.

| Date | ID | Source | Result | Evidence / limits |
| --- | --- | --- | --- | --- |
| 2026-10-30 | S5-U05 | Base `cc3edb3dd7c92ec76fb2be742171818095fdc49f` plus [working-source manifest](logs/2026-10-30_S5-U05-manifest.json) | PASS (host only) | [Host suite output](logs/2026-10-30_S5-U05-host.txt); no physical servo/echo or bypass acceptance |

## Slow servo scan — S5-U06

Procedure defined 2026-10-30: compile wireless TEST firmware with the slow
servo slew and run `testing/pure/run.sh` for existing helper regressions.
Expect compilation and host checks to pass. Inspect that acquisition remains
in settle phase throughout movement, cancellation replaces the target, and
delayed releases advance at most one degree. No hardware or physical timing
acceptance; D05 remains pending.

| Date | ID | Source | Tester | Result | Evidence / limits |
| --- | --- | --- | --- | --- | --- |
| 2026-10-30 | S5-U06 | Base `8be851bc884ec0a31646b28242a54fcf827bb987` plus [working-source manifest](logs/2026-10-30_S5-U06-manifest.json) | Codex | PASS (build / host regressions only) | [Build](logs/2026-10-30_S5-U06-build.txt), [host output](logs/2026-10-30_S5-U06-host.txt); image checksum in manifest, not flashed; slew timing and Echo not physically tested |

## Five-degree scan readings — S5-U07

Procedure defined 2026-10-30: run the existing host helper regressions and
compile wireless TEST firmware. Expect both to pass with nine scan positions
(-20 through +20 degrees, spacing 5 degrees). Inspect that each settled median
group publishes before the next target is selected, only the centre refreshes
forward evidence, and movement never overlaps an armed measurement. Existing
physical wiring/power/calibration remain untested; no firmware execution pass.

Initial S5-U07 build / host attempts failed: the existing sweep assertion
required coarse spacing strictly greater than fine spacing. Equal spacing is
needed for the requested full five-degree sweep. Failure output retained in
[host output](logs/2026-10-30_S5-U07-host.txt) and
[build output](logs/2026-10-30_S5-U07-build.txt). The failed working tree is the
S5-U08 manifest source with `S5_COARSE_STEP_DEG > S5_FINE_STEP_DEG` rather than
`>=`; base revision is recorded with the accepted manifest below.

S5-U08 rerun procedure: allow equal coarse/fine spacing in the assertion,
then repeat the S5-U07 build / existing regression checks. Expected: PASS.

| Date | ID | Source | Tester | Result | Evidence / limits |
| --- | --- | --- | --- | --- | --- |
| 2026-10-30 | S5-U07 | Base `8be851bc884ec0a31646b28242a54fcf827bb987` plus [failed source manifest](logs/2026-10-30_S5-U07-manifest.json) | Codex | FAIL (build / host compile) | Strict coarse/fine spacing assertion; output preserved above |
| 2026-10-30 | S5-U08 | Base `8be851bc884ec0a31646b28242a54fcf827bb987` plus [working-source manifest](logs/2026-10-30_S5-U08-manifest.json) | Codex | PASS (build / existing host regressions only) | [Build](logs/2026-10-30_S5-U08-build.txt), [host output](logs/2026-10-30_S5-U08-host.txt); image checksum in manifest, not flashed; no physical scan/Echo acceptance |

## Wider scan and line-ended return — S5-U09

2026-11-02, Claude ran; host and build only, nothing flashed or driven. Source: base `79c13f1`
plus the uncommitted working tree, which also holds the barcode work of INT-132 onward.

**Finding before the change:** the real estimator, fed ideal rays from a flat box with a wall
2 m beyond, gave at most 136 mm clearance against the 300 mm needed, and from the 150 mm stop
either 0 mm (100 mm box) or no profile (150 and 200 mm boxes). So bypass could not start. Host
tests missed this because they inject clearance. A 90 degree turn at 7 mm/s also took about
13.5 s against the 12 s action limit, and a 600 mm pass about 99 s against the 90 s bypass limit.

**Change:** servo limit +/-50 degrees, as stated by the user (not bench checked in this log);
clearance from each free ray's reach; side watch at 50 degrees; bypass speeds 36 and 50 mm/s,
provisional; the first black sensor on the return ends it, then Mission advances and pivots as at
a junction. The `FORWARD` and `LINE` routes are gone.

**Defined before the run:** the host suite must pass with a full sweep across 100-200 mm boxes
from 110-150 mm, and a bypass simulated with a ray cast against the box instead of fixed ranges.
Both images must build.

**Result:** [host suite](logs/2026-11-02_S5-U09-host.txt) PASS, [builds](logs/2026-11-02_S5-U09-build.txt)
PASS. Not a physical pass: the rays are ideal, without the real beam's spread. Servo centre, sonar
mount, speeds, settle time and open-floor echoes remain D05/D07. MR-30 and MR-31 are not run.

## Servo and sonar bench — S5-U10

Procedure defined 2026-11-02, motors inhibited (`./flash.sh test`), before any bypass run. Covers
MR-17 and MR-18. The test image prints `[SCAN]` lines for each ray and the resulting profile.

**Expected:** the horn clears the car at both limits and the servo does not stall; 0 degrees faces
forward; positive angles look left; ranges agree with a tape; a box gives a profile.
**Record:** centre offset, actual travel at each limit, range error at each distance, what open
directions return, and the rays for each box.

**Result:** PENDING, not run.

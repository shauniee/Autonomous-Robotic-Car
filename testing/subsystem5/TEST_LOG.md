# S5 test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)
- [2026-10-27 — Minimum-bit obstacle packing candidate (#111)](#2026-10-27--minimum-bit-obstacle-packing-candidate-111)
- [Documentation alignment — 2026-10-29](#documentation-alignment--2026-10-29)

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

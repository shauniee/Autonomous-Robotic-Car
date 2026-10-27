# S5 test log

Owner: Jia Jing / obstacle. Inactive placeholder. Retired template definitions: S5-U01 task-context example; S5-U02 unfilled manual placeholder. No template result was recorded. No subsystem hardware test is claimed.

## Contents

- [Test cases](#test-cases)
- [Results](#results)

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

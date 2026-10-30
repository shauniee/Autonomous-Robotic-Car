# S4 test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)
- [2026-10-27 — Minimum-bit TerrainContext candidate (#110)](#2026-10-27--minimum-bit-terraincontext-candidate-110)
- [Documentation alignment — 2026-10-29](#documentation-alignment--2026-10-29)

Owner: Darrius / terrain. Inactive placeholder. Retired template definitions: S4-U01 summary message-buffer bound; S4-U02 LED pause/resume every six reports. No template result was recorded. No subsystem hardware test is claimed.

## Test cases

Project feature tests: PENDING until the subsystem is implemented.

## Results

No project results recorded.

## 2026-10-27 — Minimum-bit TerrainContext candidate (#110)

| Date | ID | Source | Tester | Result | Evidence | Notes |
|---|---|---|---|---|---|---|
| 2026-10-27 | S4 CR-110 host float-reference replay + 4 profile builds | `e8f5a8d9` | GitHub Actions | PASS | [Workflow run 40518795052](https://github.com/shauniee/Autonomous-Robotic-Car/actions/runs/40518795052) | Synthetic flat/hump/impact/calibration scenarios compared with original float reference; NOT real IMU measurements |
| 2026-10-27 | S4 CR-110 Cortex-M0+ sizeof probe | `e8f5a8d9` | arm-none-eabi-gcc (Docker build image) | PASS | Same workflow, Terrain sizeof step | `TerrainContext` 32 bytes, `TerrainCalibSum` 9 bytes (3 x 23-bit signed), `TerrainFilterQ16` 12 bytes |

All D06 sensor offsets, hump scale and acceptance thresholds remain provisional. Physical calibration and measured timing are required before declaring the algorithm safe for final hardware use.

## Documentation alignment — 2026-10-29

Planning note only; no test was executed and no historical result is changed.
The [central plan](../../plan.md) owns future implementation sequence;
[mission-readiness](../mission-readiness.md) owns target physical procedures.
Earlier task splits, GP20 reset flows, JSON messages, watchdog assumptions and
configured timing targets describe their recorded revisions. Do not interpret
them as target passes. Use the applicable procedure revision, exact source and
new evidence filenames for reruns; never overwrite evidence or reuse retired IDs.

# S4 test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)
- [2026-10-27 — Minimum-bit TerrainContext candidate (#110)](#2026-10-27--minimum-bit-terraincontext-candidate-110)
- [Documentation alignment — 2026-10-29](#documentation-alignment--2026-10-29)
- [Boot calibration diagnostics — 2026-10-30](#boot-calibration-diagnostics--2026-10-30)

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

## Boot calibration diagnostics — 2026-10-30

User reports dashboard calibration failure after reset; no physical calibration
pass is inferred. The working change increases the software attempt budget from
2 to 5 seconds and logs initialization failure, completion or timeout sample count.

| ID | Check | Result | Evidence |
| --- | --- | --- | --- |
| S4-U03 | Wireless TEST compile after changing the boot budget and diagnostics | FAIL: linker could not find generated objects after profile invalidation | [Build output](logs/2026-10-30_S4-U03-build.txt) |

S4-U04: retry the explicit wireless TEST profile after generated-object
invalidation; expect a linked ELF and generated UF2. This is a compile check,
not an executed firmware or physical calibration test. Source identity and final
result are recorded in the accompanying manifest. Host regression suite
`sh testing/pure/run.sh` passed, including terrain replay and MQTT decoding;
it does not exercise actual I2C or boot scheduling.

S4-U04 result: **BUILD PASS** on retry, base commit
`8be851bc884ec0a31646b28242a54fcf827bb987` plus uncommitted source hashes in the
[manifest](logs/2026-10-30_S4-U04-manifest.json), tester Codex.
[Build output](logs/2026-10-30_S4-U04-build.txt). S4-U03 used the same firmware
source state. Existing WiFi diagnostics changes were retained. The resulting
wireless TEST UF2 inhibits motors; it has not been flashed or run on hardware.

S4-U05 procedure (2026-11-02): calibrate with 100 flat synthetic samples,
feed a 20-degree climb, then a -60-degree descent whose filtered pitch skips
the crest band between samples, then flat ground. Expect DESCENDING on the
second descent sample and one valid completed hump. Run existing terrain
regressions as well. This is host-only evidence, not measured ramp geometry.

S4-U05 result: **HOST PASS**, tester Codex, base HEAD
`4b1cc197c2fb25031ef5c5dae67b8afd8240299b` plus working-tree SHA256 identities in
[evidence](logs/2026-11-02_S4-U05-host.txt). The new regression failed before
the fix and passes afterward. No firmware or physical pass claimed.

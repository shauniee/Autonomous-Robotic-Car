# Robot completion plan

## Contents

- [Consolidated tracker](#consolidated-tracker)
- [Status and source baseline](#status-and-source-baseline)
- [Remote implementation inventory](#remote-implementation-inventory)
- [Implementation stages](#implementation-stages)
- [Owning guides](#owning-guides)
- [Full-plan continuation audit](#full-plan-continuation-audit)
- [Software implementation — 2026-10-29](#software-implementation--2026-10-29)

## Consolidated tracker

[Issue #117 — Robot completion plan and remaining acceptance](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117)
is the **single live plan and unfinished-work checklist**. It contains the
completed software inventory, ordered remaining stages, acceptance/evidence
requirements and closure rule. Update that issue as work progresses; this file
is its repository entry point, not a second checklist.

The issue consolidates #129–#133, the physical work already assigned to #117,
and unfinished calibration/approval/submission work from older closed issues.
PR #115 targets the earlier task layout; its useful measurement ideas are
retained as work in #117, without merging obsolete instrumentation.

## Status and source baseline

Consolidation baseline: main `565bae7e95668d172fe4b632a14209afc4b4b548`,
2026-10-31. Most feature software and the three-application-task refactor are
implemented. Host checks and four firmware build profiles have recorded passes;
physical calibration, timing/load validation, bypass release and repeatable
mission acceptance remain incomplete. No build is a physical pass.

The [previous full plan](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/plan.md)
is preserved in Git history. Its old remote/local snapshots and stage numbers
remain the meaning of historical evidence, not the current source inventory.
Current requirements/decisions live in [project specs](docs/project-specs.md).

## Remote implementation inventory

The [completed software section](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117#completed-software)
is the current plan inventory. Code lives in `app_program/mission/`,
`app_program/common/` and the five `app_program/subsystemN/` folders.
[Architecture](docs/architecture.md#tasks) owns task/core responsibilities;
[robot_config.h](app_program/common/robot_config.h) contains actual provisional
scheduling/control settings. The issue separates implemented helpers from
physical acceptance and disabled features.

## Implementation stages

Follow the [ordered stages in #117](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117#stage-0-decisions-and-final-test-setup):
decisions/setup, electrical/boot/local control, sensor/Motion calibration,
barcode/junction acceptance, Terrain references, obstacle/bypass release,
network/fault/real-time acceptance, then mixed mission/submission evidence.
Complete prerequisites and agree missing limits before the corresponding PASS.
The [issue completion rule](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117#completion-rule)
requires actual evidence and three successful unchanged-image observation
sessions; it introduces no automatic finish marker.

## Owning guides

| Facts | Owner |
| --- | --- |
| Live plan, work order and remaining completion status | [Issue #117](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117) |
| Requirements, data widths, accepted choices and pending limits | [Project specs](docs/project-specs.md) |
| Tasks, authority, sharing, fault/freshness policy and MQTT schema | [Architecture](docs/architecture.md) |
| Team, pins/wiring and coding rules | [Team](docs/team.md) |
| Build, flash and operating commands | [README](README.md) |
| How to record tests and preserve evidence | [Testing](testing/README.md) |
| Ordered physical procedures | [Mission readiness](testing/mission-readiness.md) |
| Submission evidence and video index | [Week 13](docs/week13-plan.md) |

## Full-plan continuation audit

The full plan remains **BLOCKED** until #117's acceptance is complete.
`python3 testing/check_plan_completion.py` checks the latest appended,
source-pinned stage audit; it does not fetch issue checkboxes, execute firmware
or certify hardware. Exit 1 means current evidence still reports BLOCKED;
exit 2 means the manifest no longer matches source and needs an appended audit.
Preserve historical records; never rewrite them to manufacture completion.

## Software implementation — 2026-10-29

This heading preserves links to the historical restructure stage record. See
the [integration log](testing/integration/TEST_LOG.md#software-restructure--2026-10-29)
and the [previous plan](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/plan.md#software-implementation--2026-10-29)
for its original source identities. Current implementation and outstanding
acceptance are tracked in [#117](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117); the original local branches have
since been integrated into main.

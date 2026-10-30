# Full robot implementation plan

## Contents

- [Status and source baseline](#status-and-source-baseline)
- [Documentation ownership](#documentation-ownership)
- [Accepted behavior](#accepted-behavior)
- [Remote implementation inventory](#remote-implementation-inventory)
- [Implementation stages](#implementation-stages)
  - [Stage 0: Reconciled contracts — issue 129](#stage-0-reconciled-contracts--issue-129)
  - [Stage 1: Independent Motion stop path — issue 130](#stage-1-independent-motion-stop-path--issue-130)
  - [Stage 2: Bounded Mission coordinator — issue 131](#stage-2-bounded-mission-coordinator--issue-131)
  - [Stage 3: Bounded I/O and cross-core contracts — issue 132](#stage-3-bounded-io-and-cross-core-contracts--issue-132)
  - [Stage 4: Integration and vertical slice — issue 133](#stage-4-integration-and-vertical-slice--issue-133)
  - [Stage 5: Motion and line calibration](#stage-5-motion-and-line-calibration)
  - [Stage 6: Barcode and junction navigation](#stage-6-barcode-and-junction-navigation)
  - [Stage 7: Terrain and hump behavior](#stage-7-terrain-and-hump-behavior)
  - [Stage 8: Obstacle profiling and bypass](#stage-8-obstacle-profiling-and-bypass)
  - [Stage 9: Mixed mission and submission evidence](#stage-9-mixed-mission-and-submission-evidence)
- [Feature acceptance matrix](#feature-acceptance-matrix)
- [Decisions before physical release](#decisions-before-physical-release)
- [Delivery and review](#delivery-and-review)
- [Software implementation — 2026-10-29](#software-implementation--2026-10-29)
- [Full-plan continuation audit](#full-plan-continuation-audit)

## Status and source baseline

Planning revision: 2026-10-29. This document is the single implementation roadmap;
it does not claim that the target is implemented or physically accepted.
Remote `main` was verified through GitHub's branch API on 2026-10-29 as
`5e6a45b768e7e2c2f8d69ba18c0360b6680d87de`. Source inspection uses that Git object.
The working branch also contains local checkpoint
`4b00e20981c0eb7b2012f6dbeeacfb4cb5387730` (tuning, dashboard and evidence), plus
pre-existing documentation drafts. Those changes are not remote-main behavior.
Before firmware work, refresh `main` and review the checkpoint diff; do not
silently import tuned constants, sign changes or claimed physical results.

Sources: latest user decisions and supplied professor clarification/review,
reconciled with [requirements](docs/project-specs.md#source-provenance-and-requested-design).
The professor asks for a simple vertical slice, explicit contracts and measured
latency, interrupt load, height error and bypass success. Historical leases,
core swaps, automatic COMPLETE and turn-then-undo navigation are superseded.

## Documentation ownership

| Owner | Facts maintained there |
| --- | --- |
| This plan | Work sequence, dependencies, file impact and acceptance mapping |
| [Project specs](docs/project-specs.md) | Requirements, source provenance, minimum-bit table and the only pending-decisions register |
| [Architecture](docs/architecture.md) | Tasks, authority, states, stop/freshness policy, sharing and MQTT schema |
| [Team](docs/team.md) | People, pins, wiring, kernel-object inventory and coding rules |
| [Subsystem guides](docs/architecture/) | Internal algorithms, existing code inventory and subsystem-specific work |
| [README](README.md) | Commands and operating instructions, explicitly labelled current versus target |
| [Testing](testing/README.md) | Stable test IDs, evidence recording and future physical procedures |

Link to owning facts rather than duplicating mutable tables. Historical ADRs,
Tribunal notes and test results remain historical; they are not alternative
current plans. Update guides alongside each implementation PR.

## Accepted behavior

The authoritative contracts are [tasks](docs/architecture.md#tasks),
[Mission loop](docs/architecture.md#mission-loop),
[fault policy](docs/architecture.md#fault-bitmask) and
[course rules](docs/project-specs.md#course-rules).
In summary: retain five subsystem modules, micro T-Kernel, C/Pico SDK and MQTT
schema v2; run three application tasks without a core swap; Motion outranks
Mission and alone writes motors. Barcode actions occur once at the next detected
junction and follow the outgoing route. Runs loop until local stop or a stopping
guard. No automatic finish and no action reversal.

Do not add a generic event bus, scheduler, route planner, recovery framework or
new watchdog framework. GPIO capture remains short and under the existing
core-1 dispatcher. Networking stays in the kernel WiFi service context.

## Remote implementation inventory

These are code-inspection findings at the pinned remote commit, not robot passes.

| Area | Reusable implementation | Gap to target |
| --- | --- | --- |
| Startup | `app_program/app_main.c`; five subsystem init/start paths | Separate init from exactly three task creations; required-control failure inhibits outputs |
| Tasks | Reporting, Motion, IR/Barcode, Safety, Mission, Terrain and Obstacle on core 1; WiFi/USB core 0 | Merge bounded sensing/navigation into Mission; Terrain/Reporting into core-0 I/O; retire Safety only after Motion protection exists |
| Motion | `subsystem2.c`, `motion_control.c`, encoder/motor drivers, packed command/health, expiry/stall and stop flag | Independent GP21 path, mode-dependent freshness, revised local reset, verified signs/geometry/control |
| Mission | `subsystem3/mission*.c/.h`, seven older state codes including CALIBRATE/COMPLETE | Extract coordinator; replace state set; preserve useful bounded action/bypass logic |
| Line/barcode | ADC calibration, snapshots, fixed capture/event storage, A-Z decoder and mapping | Bounded step interface, physical barcode adapter/configuration, junction qualification and exact-once navigation |
| Terrain | GY-511 driver, stationary calibration, filtered pitch, encoder-based hump integration and packed outputs | Bounded core-0 step, acquisition/progress age, startup sequencing, run reset and physical calibration |
| Obstacle | Echo ISR, servo, coarse/fine scan/profile and side-watch code | Filtered edge estimation, measured clearance/uncertainty, reliable bypass and original-line confirmation |
| Reporting | Binary frames, installed-mapping ACK, events and fixed TX/RX rings | Core-0 bounded step; cross-core mapping/event ownership and bounded work under continuous RX |
| Evidence | Host fixtures, size probes and recorded results | Preserve all evidence; obtain final-image physical/timing acceptance |

Remote numeric settings are observations of configuration, not target choices:
Motion priority 13 / 20 ms; Mission 14 / 10 ms; Safety 11 / 10 ms; IR 12 /
10 ms with 20 ms ADC; Terrain 16 / 20 ms; Obstacle 17; Reporting 20 / 50 ms;
telemetry 500 ms; command expiry 60 ms. The local checkpoint differs in control
and tuning. Numeric target task settings await D13; do not adopt these by accident.

## Implementation stages

Each stage is an issue-sized deliverable or a set of separately scoped follow-up
issues. Existing #129-#133 provide the initial sequence; feature/calibration
work beyond their scope must receive its own issue before a branch/PR.

### Stage 0: Reconciled contracts — issue 129

- Reconcile Markdown around this plan; distinguish source requirements, user
  choices, implemented behavior and observations. Preserve protocol tables,
  field-width evidence and historical test records.
- Record unresolved choices once in the specs register. Agree freshness inputs
  for each movement mode before changing stop checks.
- Validate contents, links and anchors. This stage changes documentation only.

### Stage 1: Independent Motion stop path — issue 130

- Inventory every Safety check/caller, latch/reset path and existing watchdog
  behavior. Check GP21 in Motion on every release; honor persistent stop before
  outputs regardless of command-copy success or diagnostics congestion.
- Preserve boot/TEST/startup inhibition, motor stall/encoder fault and impact
  response, original sample times, bounded actions and command expiry.
- Implement the target GP21 release-to-ready handshake; invalidate old commands,
  require cause clearance and a separate fresh GP20 start. No remote reset/start.
- Remove Safety task/allocation only after protection is covered. Motion hanging
  remains a documented limitation; do not infer watchdog protection from tick setup.
- GP21 is then checked at Motion's 20 ms period instead of Safety's 10 ms; record
  the worst-case button-to-brake delay under D08/D13.
- Adapt existing Motion/Safety host tests: stop precedence, expiry, reset races,
  held buttons, stale-command rejection and no reset-induced movement. The
  `subsystem2.c` stop/reset runner (S2-U05) is retired: add one host test that a
  reset is refused until the brake has serviced every stop. Build
  offline/wireless, TEST/normal; record software-only results.

### Stage 2: Bounded Mission coordinator — issue 131

- Move coordination from S3 into `app_program/mission/`; keep drivers/decoder
  in their five subsystem folders. Update build inputs/includes atomically.
- Turn line acquisition/calibration, barcode service and S5 acquisition into
  bounded step APIs called by Mission. Preserve GPIO/encoder/barcode/Echo ISR
  ownership and capture storage; no waiting through turns, Echo or servo settling.
- Implement the seven target states and small internal phases. Keep safety,
  required-data checks and hazards ahead of navigation/ordinary tracking.
- Establish pending-command and encounter lifecycle; execute only at a confirmed
  junction. Add stationary U-turn pause and outgoing-line confirmation. Keep
  no-command straight crossing and requested recovery configurable.
- Keep line guidance separate from wheel PID, using actual acquisition times.
  Test fresh sample use, action IDs, cancel/timeout, junction re-entry, recovery,
  command-once semantics and no COMPLETE. Physical gates still apply.

### Stage 3: Bounded I/O and cross-core contracts — issue 132

- Add one core-0 application `io_task` for Terrain and Reporting steps; retain
  kernel WiFi/USB placement and priorities. Prevent I2C timeout paths from
  exhausting the release budget. Bound RX/event draining even during floods.
- Attempt stationary IMU calibration at boot before IR calibration; publish
  success/failure so Mission can advance after a bounded attempt. Missing IMU
  stays flagged and does not permanently block start.
- Rework mapping installation through a bounded hand-off to Mission; only
  Mission accepts/rejects a whole mapping while READY/STOPPED. ACK means installed.
- Audit S1 event log: producers become Mission/core 1 and Terrain/core 0, with
  Reporting/core 0. Local DI/EI alone is insufficient. Choose a short cross-core
  mutex/spinlock copy or bounded per-producer rings consistent with architecture;
  retain event order/time/position and document saturation/drop behavior.
- Add timestamp/progress validity beside Terrain publication without altering
  MQTT schema. Do not let repeated reads make a frozen sample appear fresh.
- Fix two known defects in this scope: Reporting pops an event before posting
  it, so every event is lost while MQTT is offline and the outgoing ring is full
  (`subsystem1.c`); and the mapping stopped-check and write can be split by a
  Mission start, installing and ACKing a mapping mid-run.
- Test bounded copies, queue fullness, partial/oversized MQTT payloads, mapping
  races, IMU failure and command rejection. Keep network APIs in WiFi context.

### Stage 4: Integration and vertical slice — issue 133

- After #130/#131/#132, create exactly three application tasks and recount all
  kernel objects/stacks. Choose numeric priorities/periods via D13 before flashing
  the target. Confirm Motion outranks Mission and every task yields.
- Verify boot startup order, network start gate, calibration/mapping, GP21 release
  reset and GP20 fresh start. Offline builds remain buildable, but the normal
  mission cannot start without WiFi and MQTT; TEST always inhibits outputs.
- Before any floor movement, raised-wheel check motor polarity and both encoder
  signs; remote main uses +1/+1, local checkpoint measured left -1 after the left
  wheel was rewired. Select signs from observed forward wheel travel, not from
  their difference in source.
- Prove line following plus timeout brake first. Measure line-to-output latency,
  required-data ages, GPIO ISR load, scheduling, stack/RAM and stop behavior
  under MQTT reconnect and maximum reporting load.
- Append host/build results with exact source/profile. Perform physical checks
  through #117; integration compilation is not physical acceptance.

### Stage 5: Motion and line calibration

- Using the signs verified in Stage 4, record counts/revolution, geometry,
  maximum edge rate, duty/speed bounds and wheel PID tuning.
- Validate distance and 90/180 degree turns, including one wheel on a hump;
  measure output-brake latency separately from physical stopping distance.
- Calibrate mounted IR under course lighting, tune line guidance after wheel
  speed, then validate curves, both-black crossing and bounded moving recovery.
- Obtain first-cut lateral/angle/recovery limits through D07/D10 before
  PASS decisions; tune constants in separate measured changes.

### Stage 6: Barcode and junction navigation

- Calibrate print scale, polarity, sensor offsets and barcode/junction geometry;
  connect the existing encoder-based decoder adapter. Verify the same character
  in both orientations; no elapsed-time-only width guess or hard-coded mapping.
- Demonstrate fresh MQTT four-letter installation without rebuild; verify all
  four actions at variable-distance junctions, including U-turn pause and no
  reversal. Confirm no-command crossing and repeat visits to a loop junction.
- Use simple encounter/quiet-zone rearm rather than a two-second blind delay;
  reject incomplete captures without inventing damaged-barcode recovery modes.
- Check stop/reset drops pending movement but retains calibration, mapping and
  last display; new mapping while stopped defines a fresh pending-action context.

### Stage 7: Terrain and hump behavior

- Validate mounted axes, stationary calibration, acquisition ages and impact
  detection. Use encoder travel and filtered pitch; no nominal-travel fallback
  when required encoder data is missing.
- Apply half normal tracking speed during a valid hump encounter, then steady
  reduced speed for measurement. Define restoration at a confirmed descent/end;
  do not count acceleration/braking artifacts as height.
- Calibrate with measured references at/below 30 mm, including the lower ramp;
  report completed heights/count/maximum and reject invalid/incomplete windows.
- Retain results on stop and reset on accepted new run as the proposed lifecycle;
  confirm under D06 before implementing that reset behavior.

### Stage 8: Obstacle profiling and bypass

- Validate sonar/servo timing, settled angles, usable range, car envelope and
  stop distance. Ordinary no echo stays unknown/report-only.
- Implement bounded repeated samples and median filtering, consecutive-angle
  edge qualification with hysteresis, and background comparison where available.
  Estimate/report widths for 100-200 mm obstacles and record error/repeatability.
- Keep the estimate separate from conservative planning using the 200 mm upper
  bound plus measured vehicle/clearance margin. Do not report the bound as a measurement.
- Scan both sides; select a supported gap, depart, follow the side at reduced
  speed, confirm the rear edge and rear-of-car clearance, return toward the
  original line, align and resume. Width/depth and events remain observable.
- Servo/chassis compensation and additional scan positions are optional
  refinements after a verified basic bypass. Never treat one lost echo as proof
  of an edge or a side observation as fresh forward coverage.
- Reverse only as a bounded, supported manoeuvre; a forward sensor supplies no
  rear clearance. Insufficient coverage, blocked paths and timeouts have explicit
  brake/stop exits. Exact retries/travel limits await D02/D05/D07.

### Stage 9: Mixed mission and submission evidence

- Exercise repeated loops with variable feature order, both barcode orientations,
  all junction actions, humps and 100-200 mm obstacles. Test stop/fault in each
  phase and network loss/reconnect while local control continues.
- Reconstruct route order from time/encoder events, measure uncertainty and
  bypass success/time, and obtain three consecutive accepted trials on unchanged
  firmware/constants. End observation sessions with GP21, not an invented marker.
- Re-measure final RAM, message sizes and minimum-bit contracts on the accepted
  source. Record WCET/jitter/ISR time/latency, margins, ring occupancy and stack
  headroom; no firmware/physical pass from a host check.
- Append all evidence and video links; never overwrite results or commit videos,
  credentials, ELF, UF2 or map files.

## Feature acceptance matrix

| Feature | Code focus | Verification / existing test family |
| --- | --- | --- |
| Startup and local control | `app_main`, buttons, Mission, Motion | MR-06..11, T02/T12/T14 |
| Wheel PID, signs and encoder actions | S2 and configuration | MR-14..15, MR-19..25, T05/T15 |
| Line guidance and crossing | S3 + Mission | MR-12, MR-26, T03, preserved S3-BF evidence |
| Sweeping line reacquisition | Mission + S3/S2 | MR-32, T11, finite travel and stable intended-line confirmation |
| Both-direction barcode + mapping | S3 + Mission + S1/kernel bridge | MR-10/13/27/28, T04/T12 |
| Stationary U-turn pause | Mission action phases | MR-28/T05; measured pause, angle and outgoing tracking |
| Hump slowdown, height/count/max | S4 + Mission/S2 | MR-16/23/29, T06/T07 |
| Width profiling and original-line bypass | S5 + Mission/S2 | MR-17/18/30/31, T08..T10 |
| Network isolation and reconnect | kernel transport + S1/I/O | MR-08/33, T12/T13 |
| Endless loop/local stop | Mission | MR-40, MR-35..38, T16 |
| Timing/memory/minimum-bit evidence | all producers and capture paths | MR-39, S3-M02, SYS-11, T14/T15 |

## Decisions before physical release

The only live register is [pending decisions](docs/project-specs.md#pending-decisions).
The 15 cm interpretation is now resolved in D16; no blind-travel permission
follows from it. Numeric task settings, +/-70 degree moving sweep
interpretation/travel cap, U-turn pause reference, scan retry/reverse bounds,
height-error target and physical timing margins must be resolved before the
corresponding movement is enabled or marked PASS. Prefer a bounded stop to
claiming unmeasured clearance or unlimited recovery.

## Delivery and review

Nicholas reviews the reconciled plan. Keep one issue per branch and one
squash-merged PR per issue, using repository branch prefixes. Reuse the existing
`docs/mission-restructure-plan` branch for this documentation work. Do not commit
to protected branches. Push, open PRs, post issue changes or merge only when asked.
Proposed updates to #117/#129-#133 are represented here and in future checklists;
this documentation edit does not publish changes to GitHub issues.

## Software implementation — 2026-10-29

The initial software restructure is implemented on local dependent issue branches;
no push, PR, merge, issue update or flash was performed. Existing documentation
and evidence drafts remain uncommitted. Remote main was freshly verified in a
clean temporary clone as the pinned `5e6a45b768e7e2c2f8d69ba18c0360b6680d87de`.
The checkpoint's control/feedforward, sign, speed and line-loop differences were
reviewed and retained as existing software, not accepted physical calibration.
Its white/white-centering grace was removed to follow the explicit line-loss policy.

| Stage | Local source / software status |
| --- | --- |
| 0 / #129 | Existing reconciled documentation retained; implementation facts updated without changing historical test bodies |
| 1 / #130 | `feat/130-motion-stop-path`, `fe39e3a5a9ef37a3a1205e7470e89e9e3a19dc30`: independent Motion stop, persistent expiry/action failure, GP21 reset, no Safety task |
| 2 / #131 | `feat/131-bounded-mission`, `b862817a40b7b75c7d2524621a1fffb5949a8f0f`: coordinator extraction, seven states, stepped line/sonar, U-turn pause and distinct outgoing acquisitions |
| 3 / #132 | `feat/132-bounded-io`, `ab54479a1b0044871cd6a6132c1747d6b3628299`: core-0 I/O, bounded calibration outcome, Terrain ages, mapping hand-off/installed ACK, mutex-protected retry-safe event log |
| 4 / #133 | `feat/133-plan-integration`, source `407cbfa362a91db81706b255664a9425be0c33f6`: explicit three-task startup, connectivity start gate, nonwaiting diagnostics, boot brake ordering, short output critical section and impact persistence; see exact source/evidence in integration log |
| 5–9 | Not complete: hardware calibration, physical vertical-slice timing/stop/stack measurements, the specified moving sweep, repeated scan filtering/edge/coverage improvements, accepted bypass and final mixed-run/submission evidence remain |

All four offline/wireless normal/TEST profiles and the host suite were exercised
for each initial stage. Evidence is in [integration results](testing/integration/TEST_LOG.md#software-restructure--2026-10-29).
No software result is a firmware execution or physical PASS. Timing defaults are
retained provisionally with explicit user authorization; D13 acceptance is open.
The requested sweep, scan retry/reverse/travel limits, 15 cm interpretation and
new-run Terrain reset are not invented. Follow-up feature branches still require
the issues and decisions specified above before implementing/enabling those stages.

## Full-plan continuation audit

User steering: the pin allocation remains as [team](docs/team.md#2-pins); D16 is
resolved in the [decision register](docs/project-specs.md#pending-decisions). The
user explicitly kept recovery/filter operating choices and the proposed Terrain
new-run lifecycle pending, and confirmed that hardware trials are unavailable.

The continuation implements fixed-storage median groups, adjacent-ray qualified
profiles, edge hysteresis and optional empty-scene comparison, edge-focused fine
angles, separate model uncertainty/planning width, actual-black junction
qualification, outgoing straight confirmation and full-width sonar acquisition
ages. All A-Z payloads now have both-orientation/two-scale fixtures. Production
scan settings are unset and bypass release is inhibited, so no unapproved
physical policy is enabled. Forward bypass movement also rejects stale/unknown
forward coverage rather than treating a side observation as clearance.

Continuation source is an uncommitted working tree on the existing integration
checkout, identified by the [INT-16 source manifest](testing/integration/logs/2026-10-29_INT-16-manifest.json),
not by base HEAD alone. No new feature branch, PR or issue publication was made;
follow-up feature branches still need their scoped issues. Existing drafts and
concurrent dashboard/Product files are preserved.

[INT-16](testing/integration/TEST_LOG.md#software-restructure--2026-10-29) records
the host suite, four firmware profiles, 35 strict application compilations and
target ABI probes. [INT-17](testing/integration/logs/2026-10-29_INT-17_completion.txt)
audits every stage and reports **BLOCKED**, not COMPLETE. Re-run
`python3 testing/check_plan_completion.py` to check source identity and the
recorded completeness status; exit 1 is expected while stages remain partial or
blocked.

The entire plan is **not completed or physically verified**. Remaining work
includes the D07 moving sweep, the D06 reset decision, accepted coverage/rear-edge/
original-line bypass behavior, calibration and measured physical timing/stack
limits, and three consecutive unchanged-image course trials. A pin map, compiled
image, synthetic profile or host replay cannot supply those measurements.

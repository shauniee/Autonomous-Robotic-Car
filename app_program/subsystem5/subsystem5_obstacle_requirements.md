# Subsystem 5: Obstacle Profiling Review

## Contents

- [Purpose](#purpose)
- [Evidence and scope](#evidence-and-scope)
- [1. Confirmed lecturer expectations](#1-confirmed-lecturer-expectations)
- [2. Suggested techniques to investigate](#2-suggested-techniques-to-investigate)
- [3. Earlier subsystem design context](#3-earlier-subsystem-design-context)
- [4. Practical code review checks](#4-practical-code-review-checks)
  - [Measurement validity and timing](#measurement-validity-and-timing)
  - [Profiling and clearance](#profiling-and-clearance)
  - [Mission and movement integration](#mission-and-movement-integration)
  - [Calibration and representation](#calibration-and-representation)
- [5. Required practical evidence](#5-required-practical-evidence)
- [6. Code inspection results (2026-10-30)](#6-code-inspection-results-2026-10-30)
  - [Obstacle profiles and what is displayed](#obstacle-profiles-and-what-is-displayed)
  - [Obstacle situation checklist](#obstacle-situation-checklist)
- [7. Review maintenance instructions](#7-review-maintenance-instructions)
- [8. Claims excluded from requirements](#8-claims-excluded-from-requirements)

## Purpose

Review existing obstacle code against requirements.
Identify gaps before changing any code.
Safe navigation is primary success criterion.

## Evidence and scope

Lecturer clarification: 25 October 2026.
Student hardware report: 24 October 2026.

This checklist uses supplied discussion content.
It also includes earlier subsystem context.
Original rubric has not been reviewed.
This is not complete rubric certification.
Implementation inspection was completed on 2026-10-30; results are recorded in
[Code inspection results](#6-code-inspection-results-2026-10-30). This review
checks source and existing host-test records, not physical acceptance.

Separate three kinds of expectations:

- **Confirmed:** lecturer explicitly stated expectations.
- **Suggested:** lecturer proposed investigation techniques.
- **Context:** earlier subsystem design decisions.
- **Review:** practical implementation verification checks.

Suggested techniques are not individually mandatory.
Equivalent methods need supporting test evidence.

## 1. Confirmed lecturer expectations

- [ ] Retain prescribed HC-SR04 ultrasonic sensor.
- [ ] Retain prescribed swivelling servo arrangement.
- [ ] Handle obstacles measuring 10–20 cm.
- [ ] Estimate obstacle span sufficiently reliably.
- [ ] Estimate surrounding clearance for navigation.
- [ ] Support safe navigation around obstacles.
- [ ] Report obstacle estimation uncertainty explicitly.
- [ ] Demonstrate consistent safe path selection.
- [ ] Demonstrate successful obstacle clearance repeatedly.
- [ ] Demonstrate successful line reacquisition afterward.

Exact physical width measurement is unnecessary.
Distinguishing every width exactly is unnecessary.
Precision metrology is not project objective.
Hardware replacement has not been approved.
The checkboxes above remain unchecked until the required behavior is supported
by physical evidence; source implementation alone does not satisfy them.

Maximum expected width may support planning.
Lecturer permits using 20 cm conservatively.
Combine that assumption with scan results.
It cannot establish unknown surrounding clearance.

## 2. Suggested techniques to investigate

Record implementation, alternative, or experimental justification.

| Technique | Review question |
| --- | --- |
| Servo settling | Are readings delayed until servo settles? |
| Repeated measurements | Are multiple readings collected per angle? |
| Median filtering | Are isolated distance spikes suppressed? |
| Background-distance subtraction | Can background returns help identify obstacles? |
| Consecutive-angle detection | Must obstacle detection persist across angles? |
| Hysteresis | Are detection transitions resistant to jitter? |
| Conservative safety margins | Does planning allow uncertainty and vehicle clearance? |
| Multiple robot positions | Can another viewpoint resolve ambiguous scans? |

Do not invent mandatory numerical thresholds.
Choose values through calibration and testing.
Document reasons for omitted suggested techniques.
Report limitations of chosen alternatives honestly.

## 3. Earlier subsystem design context

These items need repository/interface confirmation.
They are not new lecturer quotations.

- [ ] Obstacle Task coordinates sensing operations.
- [ ] Echo ISR captures echo timing.
- [ ] Maintain forward obstacle clearance monitoring.
- [ ] Produce stationary obstacle scan profiles.
- [ ] Use coarse scans for overview.
- [ ] Let coarse results guide fine scans.
- [ ] Produce safe-path proposals for Mission.
- [ ] Mission owns bypass movement decisions.
- [ ] Report scan results and progress.
- [ ] Restore servo toward forward centre.
- [ ] Treat missing echoes as unknown.

Review intended scanning sequence:

1. Command selected servo scan angle.
2. Allow calibrated servo settling time.
3. Trigger ultrasonic measurement when permitted.
4. Capture echo or bounded timeout.
5. Validate and aggregate distance readings.
6. Store angle, distance, and validity.
7. Continue coarse or targeted fine scanning.
8. Estimate span and surrounding clearance.
9. Publish proposal, uncertainty, and status.
10. Restore centre and resume monitoring.

Exact states and interfaces may differ.
Assess behaviour rather than naming conventions.

## 4. Practical code review checks

### Measurement validity and timing

- [ ] Echo waits have bounded timeouts.
- [ ] Missing echoes remain explicitly invalid.
- [ ] Invalid readings never become free space.
- [ ] Stale measurements cannot authorise movement.
- [ ] Trigger timing follows hardware constraints.
- [ ] ISR work remains short and bounded.
- [ ] Shared measurements transfer without corruption.
- [ ] Scanning permits required RTOS responsiveness.

A timeout does not prove clearance.
Neither does an isolated long-distance return.

### Profiling and clearance

- [ ] Angle orientation is clearly documented.
- [ ] Servo zero position is calibrated.
- [ ] Scan angles respect mechanical limits.
- [ ] Profiles distinguish valid and unknown readings.
- [ ] Obstacle edges resist isolated noisy readings.
- [ ] Uncertainty affects reported clearance decisions.
- [ ] Vehicle envelope informs required path clearance.
- [ ] Safety margins reflect measured uncertainty.
- [ ] Ambiguous scans trigger conservative handling.
- [ ] Both-blocked results prevent unsafe proposals.

Ultrasonic readings have broad spatial uncertainty.
Do not equate angular span with truth.
Width formulas require stated geometry assumptions.
Any calculated width remains an estimate.

### Mission and movement integration

- [ ] Scanning and movement ownership are explicit.
- [ ] Stationary scans avoid unintended vehicle motion.
- [ ] Proposals include side and clearance evidence.
- [ ] Mission receives failure and unknown outcomes.
- [ ] Mission can reject unsafe bypass proposals.
- [ ] Integration demonstrates obstacle clearance.
- [ ] Integration demonstrates line reacquisition afterward.
- [ ] Scan cancellation leaves predictable sensor state.

Subsystem 5 can provide sensing evidence.
Whole-robot success needs integration testing.
Code inspection cannot establish physical performance.

### Calibration and representation

Earlier calibration context includes these items:

- [ ] Sonar range and timing calibration.
- [ ] Servo centre and settling calibration.
- [ ] Coarse and fine scan-angle calibration.
- [ ] Bounded scan completion deadline.
- [ ] Vehicle dimensions and safety margins.

Earlier tribunal context includes compact representation:

- [ ] Review applicable minimum-bit change requirement.
- [ ] Size state fields for actual values.
- [ ] Review fault bitmask and scan flags.
- [ ] Check serialized data sizes explicitly.
- [ ] Avoid unsupported bit-field packing assumptions.

Verify tribunal scope against actual assignment.
Do not infer required layouts from memory.

## 5. Required practical evidence

Use repeated trials for each scenario.
State trial counts and operating conditions.
No numerical pass rate was supplied.
Do not invent official acceptance thresholds.

| Scenario | Expected evidence |
| --- | --- |
| Left blocked, right available | Safe right-side proposal and successful bypass |
| Right blocked, left available | Safe left-side proposal and successful bypass |
| Both sides blocked | No unsafe bypass; explicit blocked outcome |
| Missing or unreliable echoes | Unknown outcome; conservative handling |
| 10 cm obstacle | Reliable profiling and safe navigation |
| 20 cm obstacle | Reliable profiling and safe navigation |
| Intermediate obstacle width | Performance across expected width range |
| Different approach distances | Uncertainty and path decisions remain defensible |
| Ambiguous initial scan | Safe fallback or useful additional viewpoint |
| Bypass completion | Obstacle cleared and line reacquired |

For each physical trial, record:

- Actual obstacle width and placement.
- Robot starting position and orientation.
- Relevant calibrated sensor configuration values.
- Estimated span and surrounding clearance.
- Stated uncertainty and its basis.
- Proposed side and resulting decision.
- Contact, clearance, and bypass outcome.
- Line reacquisition success or failure.
- Failures, limitations, and relevant logs.

Where comparable ground truth is available:
Report estimation error across repeated trials.
Report worst observed error or spread.
Explain uncertainty method and its limitations.
Do not claim statistical confidence without evidence.

## 6. Code inspection results (2026-10-30)

Status is limited to the evidence named in each row. “Met” means that the
source-level check is present; it does not mean the robot passed a physical
trial. No runtime code was changed as part of this review.

The current source maps **Echo to GP3** and **Trig to GP2**
([robot_config.h](../common/robot_config.h#L286)), matching the wiring reported
by the user. TEST and normal builds call the same `s5_step` acquisition path
([mission.c](../mission/mission.c#L60)); TEST mode inhibits motor outputs, not
sonar reads. A numeric front range was reported in the live dashboard during
troubleshooting, but no distance, target, setup, or saved evidence was recorded,
so this is not a test result.

The S5 test log records host-only passes for range-group/profile helpers and the
servo-limit helper. It explicitly records that no subsystem hardware test is
claimed ([TEST_LOG.md](../../testing/subsystem5/TEST_LOG.md#L12)). Host fixtures
do not establish physical range accuracy, servo calibration, bypass safety, or
line reacquisition.

### Obstacle profiles and what is displayed

The firmware keeps the **latest profile**, not a saved list/history of every
obstacle. A profile is built from a stationary servo scan; it is not one of a
set of named obstacle classes.

| Existing value | What it means | Where it is available |
| --- | --- | --- |
| Front range and validity | Most recent forward sonar distance; invalid/no echo stays unknown. | Published continuously in MQTT telemetry and shown in the dashboard's Obstacle row. |
| Scan rays | Up to nine angle/range/validity samples. The scan starts with coarse angles at -20°, -10°, 0°, +10°, +20°; edge evidence may request bounded fine angles. Angles are relative to forward, positive left. | Used internally to estimate the latest profile; individual rays are not shown in the dashboard or MQTT telemetry. |
| Closest point | Estimated closest obstacle distance and angle from the profile. | Stored in the internal profile; not included in current MQTT telemetry/dashboard. |
| Width | Estimated obstacle span, not a measured ground-truth width. | Latest profile width is included in telemetry and shown in the dashboard as the first value in `Last width × depth`. |
| Left/right clearance | Estimated scan-space clearance on each side, accounting for the configured planning width and model uncertainty. | Used internally by Mission's bypass check; not shown in telemetry/dashboard. |
| Position | Estimated obstacle forward and lateral coordinates relative to the sonar/profile geometry. | Stored internally; not exposed in current telemetry/dashboard. |
| Model uncertainty and planning width | Internal estimate uncertainty and conservative width used for planning. The configured sonar uncertainty is provisional, not a calibrated accuracy rating. | Internal profile evidence only; not exposed in current telemetry/dashboard. |
| Bypass depth | Estimated distance along the obstacle when a rear-edge observation is confirmed. | Latest depth is included in telemetry and shown as the second value in `Last width × depth`; this is not a history of obstacles. |
| Scan state and ID | Idle, settling, coarse, fine, done, failed, cancelled, or side-watch, with a small scan ID. | Internal Mission/S5 interface; not shown in current dashboard. |

During each stationary scan, the firmware takes a bounded group of three pings
per angle, requires at least two valid readings, and uses the median. These
are current software settings, not physical calibration results. The scan can
produce no profile if the centre ray or qualified, bracketed edge evidence is
missing.

**Important current limitation:** `MISSION_BYPASS_RELEASED` is 0. Mission can
stop and scan when its obstacle trigger is reached, but it will reject the
bypass instead of driving it. Therefore profile width/depth fields are not a
record of every obstacle encountered, and the dashboard does not display a
history or all internal profile details.

### Confirmed expectations

| Requirement | Status | Evidence, gap, or next step |
| --- | --- | --- |
| Retain prescribed HC-SR04 sensor and swivelling servo | Partial | Firmware provides GP2/GP3 sonar I/O and GP12 servo control ([robot_config.h](../common/robot_config.h#L222)); installed sensor variant and physical arrangement are not evidenced here. |
| Handle 10–20 cm obstacles | Unverified | Synthetic 100/150/200 mm profiles are host-tested; no physical trial is recorded ([TEST_LOG.md](../../testing/subsystem5/TEST_LOG.md#L43)). |
| Estimate obstacle span reliably | Partial | Bounded ray estimator produces width; synthetic fixtures pass, but physical accuracy is unmeasured ([profile_edges.c](./profile_edges.c#L40)). |
| Estimate surrounding clearance | Partial | Estimator derives left/right clearance from rays and uncertainty; geometry and sensor error remain provisional ([profile_edges.c](./profile_edges.c#L127)). |
| Support safe navigation around obstacles | Partial | Mission has scan/bypass logic, but bypass release is disabled by default and no physical navigation is established ([robot_config.h](../common/robot_config.h#L172), [mission_logic.c](../mission/mission_logic.c#L547)). |
| Explicitly report estimation uncertainty | Partial | Model uncertainty is calculated internally, but is not included in MQTT telemetry; configured range error is provisional ([profile_edges.c](./profile_edges.c#L133), [subsystem1.c](../subsystem1/subsystem1.c#L85)). |
| Demonstrate consistent safe path selection | Unverified | Source contains clearance checks, but bypass is disabled and no repeated physical results exist ([bypass_control.c](../mission/bypass_control.c#L137), [TEST_LOG.md](../../testing/subsystem5/TEST_LOG.md#L12)). |
| Demonstrate repeated successful obstacle clearance | Unverified | No physical subsystem or integrated bypass trials are recorded. |
| Demonstrate line reacquisition after bypass | Unverified | Mission has a reacquisition state, but no physical bypass/reacquisition evidence is recorded ([mission_logic.c](../mission/mission_logic.c#L480)). |

### Suggested techniques

| Technique | Classification | Status | Evidence or gap |
| --- | --- | --- | --- |
| Servo settling | Suggested | Partial | Acquisition waits `SERVO_SETTLE_MS`; the 300 ms value is provisional and unmeasured ([subsystem5.c](./subsystem5.c#L421), [robot_config.h](../common/robot_config.h#L231)). |
| Repeated measurements | Suggested | Partial | Stationary scan groups take three samples with at least two valid; ordinary forward samples are single pings ([robot_config.h](../common/robot_config.h#L240), [range_filter.c](./range_filter.c#L4)). |
| Median filtering | Suggested | Partial | Median is used for scan groups and has host coverage; it is not applied to ordinary forward samples ([subsystem5.c](./subsystem5.c#L264), [TEST_LOG.md](../../testing/subsystem5/TEST_LOG.md#L43)). |
| Background-distance subtraction | Suggested | Partial | Estimator accepts a background profile, but production passes `NULL`; an empty-scene reference is not collected ([subsystem5.c](./subsystem5.c#L24), [profile_edges.c](./profile_edges.c#L23)). |
| Consecutive-angle detection | Suggested | Met in code | Estimator requires adjacent qualified foreground rays and known bounding rays; synthetic cases are host-tested ([profile_edges.c](./profile_edges.c#L106)). |
| Hysteresis | Suggested | Met in code | Enter/exit thresholds are distinct and used while walking rays; settings remain provisional ([profile_edges.c](./profile_edges.c#L51), [robot_config.h](../common/robot_config.h#L246)). |
| Conservative safety margins | Suggested | Partial | Planning width includes a configured uncertainty margin and expected width; those values and the vehicle envelope need calibration ([profile_edges.c](./profile_edges.c#L133), [robot_config.h](../common/robot_config.h#L230)). |
| Multiple robot positions | Suggested | Not implemented | No second-viewpoint procedure exists; optional until a measured single-position scan works. |

### Earlier subsystem design context

| Context item | Classification | Status | Evidence or gap |
| --- | --- | --- | --- |
| Separate Obstacle task coordinates sensing | Context | Not applicable (superseded) | Current architecture calls bounded `s5_step` from Mission; no separate S5 task is required ([mission.c](../mission/mission.c#L60), [subsystem5.md](../../docs/architecture/subsystem5.md#L16)). |
| Echo ISR captures pulse timing | Context | Met in code | Rising/falling edges capture a bounded pulse duration ([subsystem5.c](./subsystem5.c#L72)). |
| Maintain forward clearance monitoring | Context | Partial | Centre-angle samples are maintained; side-watch moves the servo, and scheduled forward refresh during bypass remains outstanding ([subsystem5.c](./subsystem5.c#L286), [subsystem5.md](../../docs/architecture/subsystem5.md#L91)). |
| Produce stationary scan profiles | Context | Met in code | Scan request is gated on stationarity; movement/deadline guards fail a scan ([mission_logic.c](../mission/mission_logic.c#L520), [subsystem5.c](./subsystem5.c#L358)). |
| Coarse scan guides fine scan | Context | Met in code | Coarse rays select bounded fine angles ([subsystem5.c](./subsystem5.c#L258), [profile_edges.c](./profile_edges.c#L201)). |
| Produce safe-path proposals | Context | Partial | Profile contains left/right clearance and Mission checks it; release remains disabled pending accepted policy ([profile_edges.c](./profile_edges.c#L168), [mission_logic.c](../mission/mission_logic.c#L547)). |
| Mission owns movement decisions | Context | Met in code | Mission evaluates profile and bypass state; Motion receives movement commands ([mission_logic.c](../mission/mission_logic.c#L455)). |
| Report scan results and progress | Context | Partial | Scan state/ID are shared internally; scan phase and uncertainty are not exposed in current telemetry ([subsystem5.c](./subsystem5.c#L28), [subsystem1.c](../subsystem1/subsystem1.c#L85)). |
| Restore servo toward centre | Context | Met in code | Scan completion/cancellation recentres; physical centre is uncalibrated ([subsystem5.c](./subsystem5.c#L210)). |
| Treat missing echoes as unknown | Context | Met in code | Invalid code/valid bit are preserved; unknown rays cannot qualify a profile ([echo_range.h](./echo_range.h#L8), [profile_edges.c](./profile_edges.c#L9)). |

### Measurement validity and timing

| Check | Classification | Status | Evidence or gap |
| --- | --- | --- | --- |
| Echo waits have bounded timeouts | Review | Met in code | Echo capture times out at `SONAR_TIMEOUT_US` ([subsystem5.c](./subsystem5.c#L429), [robot_config.h](../common/robot_config.h#L218)). |
| Missing echoes remain explicitly invalid | Review | Met in code | Timeout maps to `ECHO_RANGE_INVALID`; packed validity is cleared ([echo_range.h](./echo_range.h#L26), [subsystem5.c](./subsystem5.c#L438)). |
| Invalid readings never become free space | Review | Partial | Estimator and bypass require valid evidence; ordinary line-following does not stop solely because range is unknown, by design ([profile_edges.c](./profile_edges.c#L9), [mission_logic.c](../mission/mission_logic.c#L402)). |
| Stale measurements cannot authorize movement | Review | Met in code | Mission ages samples with full timestamps; bypass checks age and sequence ([mission.c](../mission/mission.c#L135), [bypass_control.c](../mission/bypass_control.c#L125)). |
| Trigger timing follows sensor constraints | Review | Met in code; hardware unverified | Code drives a 10 µs trigger-high pulse; electrical timing has not been measured ([subsystem5.c](./subsystem5.c#L131)). |
| ISR work remains short and bounded | Review | Met in code | Handler records edge timestamps/state only ([subsystem5.c](./subsystem5.c#L72)). |
| Shared measurements transfer without corruption | Review | Met in code | Reporting mirror is copied under a mutex; Mission reads same-task acquisition records ([subsystem5.c](./subsystem5.c#L463)). |
| Scanning permits RTOS responsiveness | Review | Met in code | Settle, Echo, and scan phases advance per Mission release; only the trigger pulse uses a short wait ([subsystem5.c](./subsystem5.c#L146)). |

### Profiling and clearance

| Check | Classification | Status | Evidence or gap |
| --- | --- | --- | --- |
| Angle orientation is documented | Review | Met in code | Side-watch angle is relative to forward, positive left ([subsystem5.h](./subsystem5.h#L29)). |
| Servo zero position is calibrated | Review | Unverified | Centre pulse is provisional; no measured alignment result is recorded ([robot_config.h](../common/robot_config.h#L228)). |
| Scan angles respect mechanical limits | Review | Partial | Software clamps to configured ±20°; physical travel/clearance is unverified ([subsystem5.c](./subsystem5.c#L99), [robot_config.h](../common/robot_config.h#L230)). |
| Profiles distinguish valid and unknown readings | Review | Met in code | Each ray carries validity; invalid rays break qualified profile continuity ([profile_edges.c](./profile_edges.c#L9)). |
| Obstacle edges resist isolated noise | Review | Met in code | Adjacent rays, enter/exit hysteresis and host fixtures are present; physical noise behavior is unverified ([profile_edges.c](./profile_edges.c#L51), [TEST_LOG.md](../../testing/subsystem5/TEST_LOG.md#L43)). |
| Uncertainty affects clearance decisions | Review | Partial | Unknown uncertainty prevents clearance output; production uncertainty is a proposed 10 mm, not measured ([profile_edges.c](./profile_edges.c#L133), [robot_config.h](../common/robot_config.h#L249)). |
| Vehicle envelope informs required clearance | Review | Partial | Width/length/clearance constants are used, but remain provisional and need measurement ([bypass_control.c](../mission/bypass_control.c#L137), [robot_config.h](../common/robot_config.h#L230)). |
| Safety margins reflect measured uncertainty | Review | Unverified | Margins exist in code but are not physically calibrated. |
| Ambiguous scans trigger conservative handling | Review | Met in code | Failed/invalid profiles stop Mission; bypass release is disabled ([mission_logic.c](../mission/mission_logic.c#L531), [robot_config.h](../common/robot_config.h#L172)). |
| Both-blocked results prevent unsafe proposals | Review | Partial | Bypass validation rejects inadequate clearance, but no physical or complete integration result is recorded ([bypass_control.c](../mission/bypass_control.c#L137)). |

### Mission and movement integration

| Check | Classification | Status | Evidence or gap |
| --- | --- | --- | --- |
| Scanning and movement ownership are explicit | Review | Met in code | Mission coordinates scans; Motion owns drive commands ([mission.c](../mission/mission.c#L60), [subsystem5.md](../../docs/architecture/subsystem5.md#L16)). |
| Stationary scans avoid unintended vehicle motion | Review | Met in code | Scan starts only when stationary and fails if encoders move ([mission_logic.c](../mission/mission_logic.c#L520), [subsystem5.c](./subsystem5.c#L358)). |
| Proposals include side and clearance evidence | Review | Met in code | Profile returns left/right clearances and planning width; Mission passes them to bypass validation ([profile_edges.c](./profile_edges.c#L168), [mission_logic.c](../mission/mission_logic.c#L547)). |
| Mission receives failure and unknown outcomes | Review | Partial | Scan failure/cancel reaches stop logic; unknown forward range is invalid but does not itself trigger an obstacle scan ([mission_logic.c](../mission/mission_logic.c#L531), [mission.c](../mission/mission.c#L135)). |
| Mission can reject unsafe bypass proposals | Review | Met in code | Geometry and minimum clearance are checked; bypass release remains disabled ([bypass_control.c](../mission/bypass_control.c#L137), [mission_logic.c](../mission/mission_logic.c#L547)). |
| Integration demonstrates obstacle clearance and line reacquisition | Review | Unverified | No physical subsystem/integration trials are logged ([TEST_LOG.md](../../testing/subsystem5/TEST_LOG.md#L12)). |
| Scan cancellation leaves a predictable sensor state | Review | Met in code | Cancellation marks scan cancelled, cancels Echo and recentres servo ([subsystem5.c](./subsystem5.c#L346)). |

### Calibration and representation

| Check | Classification | Status | Evidence or gap |
| --- | --- | --- | --- |
| Sonar range and timing are calibrated | Review | Unverified | No measured range/timing result; constants remain provisional. |
| Servo centre, settling, and scan angles are calibrated | Review | Unverified | No physical calibration results are recorded. |
| Scan completion deadline is bounded | Review | Met in code | Deadline is checked and scan failure is explicit ([subsystem5.c](./subsystem5.c#L358), [robot_config.h](../common/robot_config.h#L280)). |
| Vehicle dimensions and safety margins are measured | Review | Unverified | Current values are provisional; no measurements are recorded. |
| Minimum-bit change, state/fault/scan representation, serialized sizes | Review | Met for recorded software checks | S5 packing candidate and helper/ABI checks have host/compile evidence; no physical acceptance follows ([TEST_LOG.md](../../testing/subsystem5/TEST_LOG.md#L22), [TEST_LOG.md](../../testing/subsystem5/TEST_LOG.md#L50)). |

### Practical trial status

The following all remain **Unverified**: left-blocked/right-available bypass;
right-blocked/left-available bypass; both-sides-blocked fallback; missing or
unreliable echoes; 10 cm, 20 cm, and intermediate-width obstacles; different
approach distances; ambiguous-scan fallback; and completed bypass with line
reacquisition. Existing host tests use synthetic fixtures only. Record each
physical result using the procedure in [Required practical evidence](#5-required-practical-evidence);
do not convert the informal dashboard reading mentioned above into a pass.

### Obstacle situation checklist

Use this as the practical checklist for what the robot should do. Leave each
box unchecked until that situation has been run on the robot and its result is
recorded. **No physical obstacle situation below is currently recorded as
passed.** Host tests exercise synthetic data and logic only; they do not count
as a robot pass. Bypass movement is disabled by default, so bypass scenarios
cannot pass with the current default firmware.

| Situation to test | Expected behaviour | Robot pass? / current evidence |
| --- | --- | --- |
| Clear track, no obstacle ahead | Continue following the line; keep reporting the live front range. Do not start a bypass. | [ ] Not physically verified. The earlier dashboard range observation has no saved setup or test record. |
| Obstacle blocks the line | Slow/stop safely, wait until stationary, then request a scan and form a profile or fail explicitly. | [ ] Not physically verified. The stop distance and stationary timing are configured, not physically accepted. |
| Obstacle is 10 cm wide | Profile the obstacle and choose only a path with enough measured clearance for the whole car plus margin. | [ ] Not physically verified. Synthetic 100 mm profile fixture is host-tested only. |
| Obstacle is 15 cm wide | Same safe profile and clearance decision; report an estimate, not ground-truth width. | [ ] Not physically verified. Synthetic 150 mm profile fixture is host-tested only. |
| Obstacle is 20 cm wide | Same safe profile and clearance decision at the upper stated width. | [ ] Not physically verified. Synthetic 200 mm profile fixture is host-tested only. |
| Left side blocked, right side open | Reject the blocked side; if right clearance is sufficient, bypass to the right while observing the obstacle. | [ ] Not physically verified; bypass is disabled by default. |
| Right side blocked, left side open | Reject the blocked side; if left clearance is sufficient, bypass to the left while observing the obstacle. | [ ] Not physically verified; bypass is disabled by default. |
| Both sides blocked or clearance too narrow | Do not start a bypass; stop or report a blocked/failure outcome rather than risk a collision. | [ ] Not physically verified. Synthetic geometry checks do not prove physical stopping behavior. |
| No echo, intermittent echo, or stale range | Treat the reading as unknown, never as proof that the path is clear; do not authorize a bypass from it. Check the current forward-monitor limitation below. | [ ] Not physically verified. Current review notes that an unknown forward range alone does not trigger an obstacle scan, so this case needs explicit integration verification. |
| Noisy or ambiguous scan / scan timeout | Do not make a confident profile from inadequate rays; retry only where the scan policy permits, otherwise stop safely. | [ ] Not physically verified. Invalid-profile and scan-guard behavior has software evidence only. |
| Robot moves during stationary scan, or scan is cancelled | Cancel/reject the scan, avoid using its result, and leave the sensor in a predictable state. | [ ] Not physically verified. Stationary and cancellation guards are present in code; no target test is recorded. |
| Different approach distances or obstacle placements | Keep estimates and decisions defensible; reject profiles that lack required edge/clearance evidence. | [ ] Not physically verified. No range/placement calibration results are recorded. |
| Bypass is underway and the obstacle has not yet been passed | Continue observing; do not declare the obstacle cleared based only on the initial scan. Stop safely on invalid observations or a route failure. | [ ] Not physically verified; bypass is disabled by default and forward-refresh coverage remains outstanding. |
| Rear edge is detected and obstacle is passed | Record the estimated bypass depth, return toward the original line, reacquire it, and resume line following. | [ ] Not physically verified; no integrated bypass/reacquisition run is recorded. |

When a case is run, replace its unchecked status only after adding the date,
firmware/source revision, setup, expected result, observed result, and evidence
file to the testing records. A successful scan, dashboard value, or host test
alone is not a pass for safe obstacle avoidance.

### Review conclusions and next steps

1. **Already implemented and software-tested:** asynchronous bounded Echo capture,
   invalid/no-echo representation, range groups/median, stationary coarse/fine
   scans, profile/clearance estimation, hysteresis, scan guards, and compact S5
   representations. Evidence is host-only and source-pinned in the S5 log.
2. **Main blocker:** `MISSION_BYPASS_RELEASED=0`; do not enable bypass based only
   on code or synthetic fixtures. Forward-coverage scheduling, calibrated
   uncertainty/geometry, accepted rear-edge handling, and original-line return
   still need work and physical evidence.
3. **Next minimal work:** measure sonar and servo behavior; agree and record
   calibration/vehicle-envelope values; test failure and both-blocked cases;
   complete supervised integration runs for both sides and line reacquisition;
   only then review whether bypass release can be enabled.
4. **Still needs confirmation:** compare this checklist against the original
   assignment rubric; lecturer clarifications alone do not certify every rubric
   item.

## 7. Review maintenance instructions

Use this document as review checklist.
Inspect repository before proposing implementation changes.
Do not mark physical checks complete from source inspection or host tests.
Reinspect relevant code and append dated results when implementation changes.

First identify relevant implementation files:

- Obstacle task and echo ISR.
- Servo and ultrasonic sensor drivers.
- Scan profile and filtering logic.
- Mission/bypass interfaces and state transitions.
- Fault handling and compact data representations.
- Existing calibration, logs, and tests.

For future reviews, record each checked item using:

| Item | Classification | Status | Evidence, gap, or next step |
| --- | --- | --- | --- |
| Example | Confirmed / Suggested / Context / Review | Met / Partial / Missing / Unverified / Not applicable | Link to implementation/test evidence and state the smallest justified next step |

Apply status meanings consistently:

- **Met:** sufficient relevant evidence exists.
- **Partial:** behaviour exists with identified gaps.
- **Missing:** required behaviour is demonstrably absent.
- **Unverified:** evidence cannot establish the behaviour.
- **Not applicable:** justify why scope excludes it.

Suggested techniques can use justified alternatives.
Absent test records mean unverified performance.
Function names alone are insufficient evidence.
Simulation alone cannot establish physical reliability.

Finish future reviews with these deliverables:

1. Summarise confirmed requirement coverage first.
2. List major safety and functional gaps.
3. Separate subsystem gaps from integration gaps.
4. List required calibration and physical tests.
5. Propose prioritised, minimal implementation fixes.
6. Identify requirements needing original rubric confirmation.

## 8. Claims excluded from requirements

Supplied lecturer reply does not establish:

- Permission to replace ultrasonic hardware.
- Mandatory exact width error tolerance.
- A fixed acceptable navigation speed.
- Permission to disregard rubric thresholds.
- Guaranteed clearance from absent sensor echoes.
- Mandatory use of every suggested technique.

Check original assignment for additional obligations.

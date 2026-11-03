## Contents

- [Team Information](#team-information)
- [Version A: Pre-Tribunal AI Ideation](#version-a-pre-tribunal-ai-ideation)
- [Week 6 Architecture Defence Snapshot Before Zoom](#week-6-architecture-defence-snapshot-before-zoom)
  - [](#)
  - [Shared Resource Allocation Matrix](#shared-resource-allocation-matrix)
  - [Subsystem Interface and Code Structure Map](#subsystem-interface-and-code-structure-map)
  - [Week 6 Assumption Closure](#week-6-assumption-closure)
- [](#)
- [Version B: Live Zoom-Updated Version](#version-b-live-zoom-updated-version)
  - [Buddy Contributions During Zoom](#buddy-contributions-during-zoom)
  - [Live Decision Change Log](#live-decision-change-log)
  - [Resource and Assumption Review](#resource-and-assumption-review)
  - [Risk Assessment and Subsystem Test Plan](#risk-assessment-and-subsystem-test-plan)
    - [Integration Risk Register](#integration-risk-register)
    - [Subsystem Test Plan](#subsystem-test-plan)
  - [Constraint Injection / Curveball Response](#constraint-injection--curveball-response)
  - [Integration Readiness Self-Assessment](#integration-readiness-self-assessment)
- [Version C: Final AI-Vetted Gap Check](#version-c-final-ai-vetted-gap-check)
- [Final Tribunal Summary](#final-tribunal-summary)

**INF2004 Embedded Systems Programming**

**Week 8 Tribunal 1 Template**

| **Item** | **Details** |
| --- | --- |
| Purpose | Defend Week 6 architecture, justify changes, and show readiness for partial integration. |
| Core workflow | Pre-submission AI ideation -&gt; live Zoom-updated version -&gt; final AI-vetted gap check |
| Assessment focus | Evidence, engineering judgement, negotiation, and ability to identify missing work |
| Submission format | Completed document plus Zoom recording and evidence links |

# Team Information

| **Item** | **Details** |
| --- | --- |
| Group ID | CSC01 |
| Repository Link | https://github.com/shaun-ie/Autonomous-Robotic-Car |

| **Buddy** | **Student ID** | **Primary subsystem / persona** |
| --- | --- | --- |
| Shaun | 2500629 | Network Gatekeeper: WiFi, MQTT, telemetry |
| Mahesha | 2503973 | Real-Time Taskmaster: motors, encoders, PID |
| Nicholas | 2501552 | Track Purist: IR sensors, line following, barcode |
| Darrius | 2500360 | Physics Watchdog: IMU, terrain, hump/motion events |
| Jia Jing | 2502825 | Spatial Optimizer: ultrasonic, servo, obstacle profiling |

<br>

# Version A: Pre-Tribunal AI Ideation

| **Buddy** | **AI prompt used** | **Useful AI ideas** | **Weak / unrealistic AI ideas** | **Position before Zoom** |
| --- | --- | --- | --- | --- |
| Shaun | Act as an embedded systems reviewer for my WiFi/MQTT subsystem. Compare our Week 6 design review with the latest code. Challenge network ownership, telemetry freshness, reconnect behavior and stopped barcode configuration. Identify useful design decisions, unrealistic assumptions, integration risks and evidence still needed. Help me prepare arguments and negotiation points for Tribunal 1 Version A. Distinguish implemented code from tested behavior. | Keep networking separate from control; acknowledge applied mappings. | Assuming a working dashboard proves robot readiness. | Retain local control that does not depend on WiFi because network delays must not interrupt driving or stopping. Keep networking separate from motor control. Mission must accept or reject mapping changes before an applied acknowledgement is sent. Reconnect and backpressure behaviour still requires validation. |
| Mahesha | Act as an embedded systems reviewer for my motors, encoders and PID subsystem. Compare our Week 6 design review with the latest code in code/. Challenge exclusive motor ownership, command leases, action completion, stationary detection, braking and reset behavior. Identify useful decisions, weak assumptions, dependencies on other subsystems and required measurements. Help me prepare my position before Zoom without treating builds as physical validation. | One motor writer; time-limited commands; retained completion results. | Assuming zero target speed means stopped, or encoder counts prove ground accuracy. | Retain Motion as the sole motor-output owner to prevent competing writes. Retain expiring commands so an old request cannot continue indefinitely. Mission must preserve the original sensor timestamp when renewing commands, and Safety must retain stop authority. Command timing and operating speed require agreement based on measured latency and stopping distance. |
| Nicholas | Act as an embedded systems reviewer for my line/barcode subsystem and Mission/Safety coordination. Compare our Week 6 design review with the latest code in code/. Challenge IR calibration, proportional guidance, barcode capture, runtime mapping, junction detection, bounded recovery and stop/reset handling. Identify useful decisions, unrealistic assumptions, shared-resource risks and missing tests. Help me prepare evidence-based arguments and negotiation points for Tribunal 1 Version A. | Separate line readings from queued barcode events; retain junction context. | Assuming two sensors identify every junction or barcode example is officially confirmed. | Retain separate line snapshots and queued barcode events because they have different delivery requirements. Mission should own navigation decisions; S3 should provide sensor evidence without writing motors. I propose bounded recovery, but the team must agree how to identify the original line and when to abandon the search. |
| Darrius | Act as an embedded systems reviewer for my GY-511 terrain and hump-measurement subsystem. Compare our Week 6 design review with the latest code in code/. Challenge sensor capabilities, stationary calibration, encoder-derived movement, hump-height estimation and maximum-height retention. Identify useful decisions, unrealistic assumptions, cross-subsystem dependencies and reference measurements needed. Help me prepare my position before Zoom, separating proposed accuracy from measured results. | Separate hump detection from height validity; retain the highest valid result. | Treating GY-511 as a gyro or chassis rise as proven hump height. | Retain separate hump detection and height validity because detecting a hump does not prove an accurate height estimate. Only a valid completed encounter should update the maximum. S2 must provide valid motion data, and the team must agree the operating speed and reference-hump test before accepting an accuracy claim. |
| Jia Jing | Act as an embedded systems reviewer for my ultrasonic, servo and obstacle-profiling subsystem. Compare our Week 6 design review with the latest code in code/. Challenge stationary scanning, servo settling, forward-range freshness, missing echoes, clearance estimates and the boundary between profiling and Mission-owned bypass. Identify useful decisions, unrealistic assumptions, integration risks and physical tests needed. Help me prepare arguments and negotiation points for Tribunal 1 Version A. | Confirm stopped wheels; keep forward range separate from side scans. | Treating no echo as clear space or sparse rays as complete clearance. | Retain stationary scanning and separate front/profile readings because side scans must not replace fresh forward obstacle information. Mission should own bypass decisions. Clearance requires measured robot geometry and uncertainty margins. No-echo handling needs an agreed retry and stop policy based on physical observations. |

# Week 6 Architecture Defence Snapshot Before Zoom

| **Week 6 ****artifact** | **What the team claimed in Week 6** | **Evidence available now** | **Still weak or untested** | **Buddy ****defender** |
| --- | --- | --- | --- | --- |
| Task table / architecture | Safety first, Motion next. Design review architecture also incorrectly labelled single core as accepted. | Current source uses dual-core micro T-Kernel: application tasks on core 1, networking on core 0. | Loaded timing and cross-core handoffs; resolve conflicting W6 Safety-owner labels. | Nicholas / Mahesha / Shaun |
| Motion state chart | Motion alone drives motors; commands expire and results remain until acknowledged. | Command, feedback, stop and acknowledgement code exists; inhibited logic checks ran. | Wheel PID, ground accuracy and physical braking. | Mahesha |
| Line/barcode state chart | Separate line readings and barcode events; decode outside interrupts; sample every 20 ms. | The S3 input task runs every 10 ms; IR ADC sampling occurs every 20 ms. Mission and Motion run every 20 ms. End-to-end response remains unmeasured. | Real print decoding, junction context and queue capacity at speed. | Nicholas |
| Communications state chart | Status-only requests; network loss must not block driving. | Stopped runtime mapping was added. Inhibited MQTT mapping changes and reboot/reload were observed. | Full transport stress and reconnect during powered operation. | Shaun |
| Terrain state chart | Only valid complete hump encounters update the maximum height. | Calibration, height-validation and history code exists; logic checks ran without the IMU. | Mounted calibration and reference-hump accuracy. | Darrius |
| Obstacle state chart | Scan while stationary; keep front and profile readings separate | Scan guards, cancellation and center restoration exist; synthetic checks ran without sonar. | Real range/profile accuracy, clearance and bypass geometry. | Jia Jing |
| Mission state chart | Reset does not start movement; recovery has time/travel limits. | Separate start/reset and guarded recovery exist; inhibited Mission checks ran. | Stop/reset races, finish rules and original-line recovery. | Nicholas |

## Shared Resource Allocation Matrix

| **Shared resource** | **Used by task(s)** | **Owner task / module** | **Access type** | **Contention risk** | **Mitigation** | **Evidence needed** |
| --- | --- | --- | --- | --- | --- | --- |
| Motors and command state | Mission, Safety, Motion | Motion owns outputs; Mission commands; Safety stops. | Command writes and feedback reads | Conflicting or expired movement requests | One motor writer; stop overrides movement; commands expire. | Physical stop, expiry and reset trials; measure the 50 ms brake-output target. |
| PWM hardware | \| Motion, buzzer, servo | Motion slices 4/5; buzzer 3; servo 6. | Owned-slice updates | Resetting PWM disrupts another output. | Separate slices; avoid resetting the shared block. | Check motor, buzzer and servo waveforms together. |
| Motion feedback | Mission, Line/Barcode, Terrain, Obstacle, Reporting | Motion | Copied snapshot | Stale/torn readings or lost completion | Check age/validity; bounded locks; retain results until acknowledged. | Concurrent-reader and lock-timing checks under load. |
| Barcode mapping | MQTT, Mission, line/barcode | S1 transport; Mission acceptance; S3 storage. | Copied request and acknowledgement | Partial updates or replacement while moving | Stopped validation; full 26-entry replacement; four acknowledgement slots. | Busy/backpressure/reconnect tests; inhibited mapping/reload subset already observed. |
| GPIO interrupts | Encoder, barcode, sonar | Shared dispatcher; module-owned pins | Edge capture | Missed edges and delayed control | Bounded handlers; deferred decoding; µT-Kernel-supported interrupt signaling. | Calculate both-wheel edge rate; measure ISR time, invalid counts and overflow under combined load. Consider PIO if necessary. |
|  |  |  |  |  |  |  |
| CPU and task memory | Application/network tasks | Team and task owners | Scheduled execution | Deadline misses, blocking and stack exhaustion | Agree numeric priorities/WCET; leave processing time beyond I²C timeout. | Measure execution, jitter, missed releases, stack use and lock duration. |
| Capture and event capacity | Line/Barcode, Mission | S3 | Capture ring and event FIFO | Overflow at speed | Separate snapshots/events; define overflow response and actual storage. | Test the 64-edge ring and eight-event FIFO; record peak occupancy and overflow. |

## Subsystem Interface and Code Structure Map

| **Module / file / interface** | **Provider** | **Consumer****s** | **Purpose** | **Inputs** | **Outputs** | **Interface rule** | **Validation rule** |
| --- | --- | --- | --- | --- | --- | --- | --- |
| S1 reporting / mapping transport | Shaun | Mission, network, laptop | Telemetry and permitted configuration | Snapshots and requests | Telemetry and acknowledgements | Define topics, JSON fields, accepted requests and retry limits; no movement authority. | Test invalid requests, reconnect and backpressure without delaying control. |
| Motion command / feedback / stop / acknowledgement | Mahesha | Mission, Safety; feedback readers | Control motors and report motion. | Leased commands, encoder counts, stop state | Motor output, measured motion, action result | Only Motion writes motors; stop/expiry overrides commands. Command acceptance, action completion, and stationary indication are separate states. | Measure speed, distance, turns, completion and braking. |
| Line snapshot / barcode event APIs | Nicholas | Mission, reporting | Provide steering guidance and navigation events. | IR readings, edges, encoder travel, mapping | Line snapshot, queued barcode event | No motor writes; preserve sample age and queued events. | Test contrast, prints in both directions, speed and duplicates. |
| Terrain snapshot / calibration APIs | Darrius | Mission, Safety, calibration coordinator, reporting | Classify movement and estimate hump height. | GY-511 samples, motion context | Calibration state, movement, height, maximum | Terrain owns processing; invalid height cannot replace a valid maximum. | Calibrate mounted sensor; compare with known humps. |
| Obstacle snapshot / scan request / cancel | Jia Jing | Mission, Reporting | Measure obstacles and clearance. | Echoes, servo settings, scan request, stationary feedback | Front range, scan rays, profile | Scan while stationary; separate front/profile age; Mission owns bypass. | Test known objects, no echo, cancellation and fresh center return. |
| Fault reporting contract | All owners | Mission, Safety, reporting | Consistent fault handling | Faults, stale data and expiry | Status and recovery eligibility | Consolidate detector, threshold, Stopped/Fault outcome, latching and recovery. | Inject faults; verify stopping and that reset never starts movement. |

## Week 6 Assumption Closure

| **Week 6 assumption ID** | **Original assumption** | **Current status** | **Evidence collected** | **Remaining risk** | **Next action** |
| --- | --- | --- | --- | --- | --- |
| A1 | WiFi/MQTT recover within the target. | Needs validation | Inhibited mapping/reload and laptop broker checks ran. | Driving reconnect time is unknown. | Shaun: interrupt WiFi/broker and measure recovery and control timing. |
| A2 | Encoder counts and direction remain reliable across speeds and reversals. | Needs validation | Manual wheel trials gave provisional counts; one trial had invalid transitions. | Count errors corrupt motion and barcode widths. | Mahesha: repeat known revolutions at several speeds and directions. |
| A3 | Mounted IR gives clear contrast and complete barcode frames. | Needs validation | Synthetic decoder/capture firmware checks passed. | Lighting, mounting and speed may cause missed/duplicate reads. | Nicholas: record real ADC ranges and barcode edges across conditions. |
| A4 | Pitch and travel can estimate calibrated hump height. | Needs validation | Logic checks without IMU. | Acceleration/slip distort height. | Darrius: calibrate, filter pitch and repeat steady-speed trials against measured humps; report error. |
| A5 | Sonar profiles represent actual obstacles. | Needs validation | Synthetic checks without sonar. | No echo may mean open space or failure. | Jia Jing: test range, no echo and sensor faults; agree retry/stop policy from observed behaviour. |
| A6 | Motor/servo load transients stay within electrical limits. | Needs validation | Power policy exists; no accepted load-transient measurements. | Brownout, damage or corrupted sensor readings. | All owners: verify modules and measure logic levels, loaded rails and currents. |
| A7 | Grove wiring and unused contacts match the design. | Needs validation | Pin risks documented; physical checks pending. | ADC, console, battery-sense and Echo conflicts. | All owners: confirm GP27=ADC1, GP0/1 stdio settings, GP28 jumper open and GP26/28 power-off continuity; measure Echo ≤3.3 V before GP2 connection. |
| A8 | Barcode mapping, orientation and junction offsets are confirmed. | Partly clarified | Four-letter rule confirmed; rehearsal mapping changes and reload observed. | Issued framing, direction, tolerances and spacing remain unresolved. | Nicholas/team: confirm course details and test issued prints. |
| A9 | The course permits bypass and original-line recovery. | Open | Recovery guards exist; no physical pass. | Ambiguous lines, blocked sides and unsuitable limits. | Nicholas/Jia Jing: define search pattern, heading/corridor, stable distance and budgets; test both-blocked, timeout and retry exhaustion. |
| A10 | Finish and hump height assessment criteria are specified. | Open | No confirmed finish convention or accepted height-error limit. | Premature finish or unsupported accuracy claims | Nicholas/Darrius: confirm rules, references and error limits. |
| A11 | Motion timing and the 50 ms brake-output target hold under peak load. | Needs validation | S3 input task: 10 ms; IR ADC sampling: 20 ms; Mission and Motion: 20 ms. These are configured periods; end-to-end response remains unmeasured. | End-to-end response remains unmeasured. | All owners: timestamp sample, decision, command and PWM under load; report brake-output latency separately from physical stopping. |
| A12 | Expiry, reset rejection and watchdog recovery prevent unsafe restart. | Needs validation | Stop/reset code and inhibited logic checks exist. | Reset or held-button behavior may allow unsafe continuation. | Nicholas/Mahesha: test expiry, old commands, held buttons, inhibited boot and stalled progress. |
| A13 | Wheel geometry and bounded slip support repeatable motion accuracy. | Needs validation | Geometry/count scaling remains provisional. | Encoder travel differs from ground travel. | Mahesha/Nicholas: calibrate wheel scaling and track width, then measure ground travel, lateral tracking and turn error against external references. Proposed targets: lateral error ≤10 mm and final turn error ≤5°. Targets and test conditions require team agreement and physical validation. |

<br>

# Version B: Live Zoom-Updated Version

## Buddy Contributions During Zoom

| **Buddy** | **Key challenge, concern, or argument raised** | **Challenge received** | **Decision or compromise reached** |
| --- | --- | --- | --- |
| Shaun | Risk of race conditions when multiple tasks update shared packed data. | How to ensure safe access to edit the data. | Use mutex protected snapshot and run concurrency tests. |
| Mahesha | When dealing with values which require precision, the number of bits needed must be decided on early on which may affect the accuracy of the motor’s movement. | For each type of value that requires precision like timestamps and speed values, how many bits are needed for safe motion controls? | Requires testing with actual robot and other subsystem’s that rely on the precision of the data. |
| Nicholas | Correct bit widths;<br><br>Independent faults need separate bits as capturing barcode and queued barcode faults could happen together;<br><br>Type of barcode is stored as unsigned int | How to justify the number of bits allocated to each field (including unknown, invalid and fault values)<br><br>How to represent multiple faults occurring simultaneously without losing any fault information? | Document every field valid value and use minimum justified widths;<br><br>Use one encoded value for each state field and independent bits for simultaneous faults. |
| Darrius | Calculation of the hump climb from decimals into small whole numbers very early may result in rounding errors to pile up over time. | How to justify the 32-bit decimal floats based on the minimum-bit representation? | Compromise by keeping the live numbers received in a 16-bit fixed-point integer for math accuracy but only packing the final result into 7 bits for storage and reporting. |
| Jia Jing | Scanning states are easier to compress than obstacle profiles which requires more considerations | How to minimize the bits used for scanning states and obstacle profiles? | Ensure to pack the obstacle profiles using minimum bits but also ensure accuracy is preserved |

## Live Decision Change Log

| **Original idea / Week 6 design** | **Updated decision** | **Change type** | **Reason for change** | **Evidence link / timestamp** | **Owner / follow-up** |
| --- | --- | --- | --- | --- | --- |
| JSON based format | Binary format | Major redesign | Meet the bit representation requirement | 47:47 | Shaun |
| Uses ordinary enums, booleans and floats for data values | Audit every field for each subsystem, restructuring the data type to binary value instead of original data types like uint\_64 | Major redesign | Meet the requirement change | 49:20 | All Owners |
| Enums for fault states | Restructure fault states to bit values | Major redesign | Meet the bit representation requirement | 51:00 | All Owners |
| Mission requests actions from all subsystems | Priorities to serve which subsystem first does not need to change | No Change | - | 54:20 | All owners |
| Priority hierarchy | - | No Change | - | - | All owners |
| Subsystems communicate through RTOS interfaces | - | No Change | - | - | All owner |
| Floating point values and signed integer values are currently normalized and handled to retain as much accuracy as possible. | Audit all related fields for each subsystem, restructuring the data type to binary whilst ensuring values are not truncated or lost | Major redesign | Meet the bit representation requirement | 58:45 | Nicholas, Mahesha, Gan Jia Jing, Darrius |

## Resource and Assumption Review

| **Item challenged** | **Evidence presented** | **Decision made** | **Impact on other buddies** |
| --- | --- | --- | --- |
| Memory usage | Existing application data uses enums, floats and integers | Compare actual RAM and message sizes before and after using the same build settings | Low |
| Core usage | Priority of subsystems which is retained | Measure cpu overhead from packing and unpacking | Low |
| Subsystem numeric values and precision | Precision of existing representation is valid | Audit and testing required from all buddies (ensure that bits are sufficient, avoiding bit overflow) | High, affects all subsystems |
| Obstacle profile size | Multiple distance and clearance | Quantize and pack measurements | Mid, affects subsystem 2, 3, 5 |
| Minimum number of bits allocated to motor encoder’s max distance | No concrete total distance of the mission | Upon each barcode scan, the encoder is requested to reset the distance to avoid overflow of values to allow robot to travel infinitely. | Mid, affects subsystems 2, 3, 5 |

## Risk Assessment and Subsystem Test Plan

### Integration Risk Register

| **Risk ID** | **Risk** | **Cause** | **Impact** | **Likelihood** | **Severity** | **Mitigation** | **Owner** | **Status** |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| R1 | Corrupted telemetry data<br><br>(Scheduling) | Concurrent updates to shared packed fields | Incorrect robot status or fault reporting | High | High | Use mutex protected snapshot and do concurrency tests | Shaun | Open |
| R2 | Range/precision loss | Widths exclude real values, invalid codes, rollover, or required resolution | Wrong motion values, freshness decisions or hump measurements | Medium | Medium | Approve bounds and precision first. Specify invalid/out of range behaviors and test boundaries | Nicholas / Mahesha / Darrius / Jia Jing | Open |
| R3 | Stack overflow | When dealing with bits, the pointers need to reference a correct memory address to avoid breaking the subsystems | Robo Pico stops its processes mid journey. | High | High | Switch to bit system incrementally to better understand the source of any memory error if occurs for easier debugging. | All buddies | Open |
| R4 | Team mismatch (Interface) | Two subsystems update different number of bits due to unregulated read-modify-write operations | Bits that could hold states or data will not be accurate across different subsystems | High | High | Use a single writer, suitable atomic operation or short critical section protecting the complete update | All buddies | Open |
| R5 | Sensor voltage incompatibility (Hardware) | Ultrasonic echo exceeds the limit 3.3V | GPIO damage or sensor failure | Medium | High | Verify Echo Voltage before connection | Jia Jing | open |

### Subsystem Test Plan

| **Subsystem** | **Test case** | **Success criteria** | **Evidence required** | **Owner** | **Status** |
| --- | --- | --- | --- | --- | --- |
| WiFi, MQTT, telemetry | Binary telemetry encoding and concurrent snapshot access | Incorrect robot status or fault reporting | Mutex protected snapshot and concurrency test | Shaun | Open |
| Motion control with encoder | Test motor movements, stopping, change direction and providing data for other subsystems | Correct reading appears in serial log. | Robo pico moves according to plan | Mahesha | Open |
| Obstacle + motion | Test stationary scanning | Scan only starts after valid stationary feedback | Timestamped sensor, motion feedback and deadline | Jia Jing | Open |
| Terrain + motion | Test hump rise estimation | Calculated height matches test hump height within a few millimeters. | Serial log showing the data | Darrius | Open |
| Motion + Tracking + Obstacle | Test for safety and timing, exercise braking, stationary confirmation, no-echo and stop cancellation | Scanning needs fresh stationary feedback | Synchronized logs | Nicholas, Mahesha, Jia Jing | Open |
|  |  |  |  |  |  |

## Constraint Injection / Curveball Response

| **Injected constraint** | **Buddy affected** | **Immediate impact** | **Adaptation / compromise** |
| --- | --- | --- | --- |
| All application data must use the minimum required number of bits | Shaun | Existing JSON telemetry changes from text to binary fields | Encode telemetry and commands in binary. Preserve mutex protected snapshots and test concurrency to prevent race conditions |
|  | Mahesha | Each buddy will have to first determine the different fields that need to be changed. Then, | Encoder and motor outputs need to be adapted to use bit representation. |
|  | Nicholas |  | IR states and IR data will be adjusted to binary representation to minimize bit usage |
|  | Darrius |  | Pack terrain states, pitch and hump height into their respective bits |
|  | Jia Jing |  | Use 3-bit states and use quantized profiles |

## Integration Readiness Self-Assessment

| **Readiness area** | **Status** | **Evidence** | **Concern** |
| --- | --- | --- | --- |
| Hardware wiring known | Green | Stated in design review, no hardware amendment is required given the change in requirement not affect it | - |
| RTOS task ownership clear | Green | Stated in design review, there is no change in task ownership given the change in requirement | - |
| Shared resources identified | Amber | Identified the shared values required by all subsystems in codebase | Tested on the new change in requirement has not been implemented. But implementation of shared resources has been identified and done up |
| Critical state charts implementable | Amber | Identified the key states required in each subsystem. |  |
| Safety or fault handling defined | Amber | Identified each subsystem’s fault states that are required. |  |
| Week 10 partial integration feasible | Amber | Existing architecture and preliminary code checks |  |

**Updated readiness position before the Week 10 partial integration demo:**

The robo pico is partially ready and has been fully integrated but no regression testing done across a few of the subsystems.  Further testing is required to verify the functionality of all the subsystem. For Minimum-bit changes, agree field ranges, packed layouts and safe access functions. Required RAM and message-size comparisons and correctness/performance **evidenc****e** is planned to be implemented before week 13 submission.

# Version C: Final AI-Vetted Gap Check

| **AI reviewer prompt used** |  |
| --- | --- |
| Review scope and prompt | Review only Versions A and B of this tribunal document. Compare the Week 6 architecture and open assumptions with the live Zoom decisions, especially the minimum-bit requirement. Challenge field ranges, precision, invalid values, simultaneous faults, shared-data ownership, binary interfaces, encoder resets, safety timing and physical readiness. Identify contradictions and missing evidence; retain justified design decisions and name the relevant owners. Distinguish recorded decisions, existing preliminary checks and proposed follow-up. Do not invent code changes, measurements, approvals or test results. Produce eight detailed gap-check entries and a final tribunal summary. This review uses the document only; the repository, recording and robot have not been independently rechecked. |

| **AI-identified gap** | **Team response** | **Action taken** | **Evidence / reason** |
| --- | --- | --- | --- |
| 1. Minimum-bit representation is not yet a complete data contract. Version B proposes smaller fields, 3-bit scan states, 16-bit terrain arithmetic and a 7-bit height result without agreed ranges, units, resolution or invalid encodings. Timestamps, signed speed, counters and simultaneous faults need separate treatment. | Retain the minimum-bit objective, but interpret it as the smallest justified representation that preserves required behavior. Binary encoding alone does not establish minimum storage. A single state value is suitable for mutually exclusive states; faults that can coexist must remain independently representable. | Recorded decision: field audit and independent fault bits. Follow-up: each owner documents bounds, units, signedness, scale, rounding, invalid codes and overflow behavior. Review packed layouts together before conversion; approve widths only after boundary and precision checks. | Version B: field audit at 49:20; fault changes at 51:00; precision review at 58:45. Nicholas explicitly identified simultaneous faults. No completed field specification or passing boundary results are supplied. Terrain and obstacle widths remain proposals requiring justification. |
| 2. Packed updates can corrupt unrelated fields or publish inconsistent snapshots. Version B retains task priorities and RTOS interfaces, but this does not prove synchronization remains safe when multiple values share storage across application and networking cores. | Retain Motion as the only motor writer, Mission as decision owner and Safety stop authority. Preserve copied snapshots and existing ownership boundaries. Protect the complete shared update; do not assume separate bit fields can be modified independently or that a mutex automatically satisfies real-time deadlines. | Recorded decision: mutex-protected snapshots and concurrency checks. Follow-up: Shaun and all providers identify every shared writer and storage unit; use one writer or an appropriate synchronized update. Measure lock duration, blocking and cross-core handoffs under combined load. | Version A describes dual-core operation, bounded locks and snapshot freshness. Version B R1 and R4 remain open, with corruption and interface mismatch rated High. Required evidence includes simultaneous updates, coherent reader snapshots, retained fault bits and unchanged control deadlines. |
| 3. Resetting encoder distance at every barcode does not establish overflow-safe travel. Missing a barcode can leave an unbounded interval; resetting a shared count can also interrupt motion targets, barcode-width measurement, terrain integration and obstacle/bypass distance tracking. | Treat Version B's reset idea as a segment-distance proposal requiring an interface review. Preserve each consumer's continuity and sign/direction meaning. A physical encoder reset, a software counter reset and a new segment origin are different operations and must not be used interchangeably. | Recorded: barcode-triggered reset was proposed. Follow-up: Mahesha coordinates with Nicholas, Darrius and Jia Jing on per-consumer baselines or another bounded counter contract. Define reset/epoch notification and rollover handling; test missed/duplicate barcodes and resets during active actions. | Version B's defence table cites unknown total mission distance and proposes a reset at each barcode. Version A identifies encoder travel as input to several subsystems, and A2/A13 remain unvalidated. No evidence demonstrates safe resets, bounded barcode spacing or indefinite travel. |
| 4. Representation changes may invalidate command freshness and stopping guarantees. Shortened timestamps, quantized speed and packing overhead can alter expiry checks or delay Safety/Motion even when the configured task periods and priorities are unchanged. | Retain original sensor timestamps when renewing commands, expiring leases, stop precedence, retained action results and separate reset/start behavior. Command acceptance, completion and measured stationary feedback remain distinct. A zero target is insufficient evidence that wheels have stopped. | Follow-up: Nicholas and Mahesha verify expiry and timestamp rollover, stale/old commands, stop/reset races, held buttons and inhibited reboot. All owners timestamp sample, decision, command and PWM under peak load, then measure stopping time/distance at agreed speeds. | Version A: S3 task 10 ms; IR sampling, Mission and Motion 20 ms; end-to-end response unmeasured. The 50 ms brake-output figure is a target, not a measured physical stop. A11/A12 and Version B's integrated safety tests remain open. |
| 5. JSON-to-binary conversion needs an explicit compatible protocol. A binary payload without field order, lengths, bit order, scaling, validity and version rules can be decoded differently by firmware and the laptop, or accepted as a malformed configuration request. | Retain local control independent of WiFi and Mission-owned acceptance of stopped mapping changes. Networking has no movement authority. Applied acknowledgements must mean the complete mapping was accepted and installed; receipt or successful decoding alone is insufficient. | Recorded: binary format selected at 47:47. Follow-up: Shaun defines the wire schema with all owners and updates both endpoints. Validate length, range and version; test malformed/truncated input, busy/retry, reconnect and full acknowledgement slots. Preserve stopped-only replacement semantics. | Version A specifies full 26-entry replacement and four acknowledgement slots; inhibited mapping changes and reboot/reload were observed. Its interface map still refers to JSON. These earlier results do not validate the new binary transport or powered-operation reconnect behavior. |
| 6. Physical line/obstacle behavior remains a gate despite packed states. Compressed data does not resolve IR contrast, real barcode framing, junction ambiguity, no echo, servo settling, actual clearance or the need for fresh stationary and forward-range feedback. | Retain separate line snapshots/barcode events and separate front/profile ages. Keep navigation and bypass decisions in Mission. Unknown/no echo must remain distinct from clear space. Recovery must have agreed heading/corridor checks, time/travel limits and a safe outcome when blocked or exhausted. | Follow-up: Nicholas and Jia Jing test issued prints, directions and speeds; record queue occupancy/overflow and scan cancellation/center return. With Mahesha, verify stationary gating and ground geometry. Before powered trials, confirm Echo level, pin/jumper wiring and loaded supply behavior. | Version A cites a 64-edge ring and eight-event FIFO, synthetic decoder/sonar checks, open course/finish rules and pending wiring/load checks. Version B R5 remains open (Echo above 3.3 V), so its Green wiring label does not close the documented electrical checks. |
| 7. Terrain precision and height validity are still unproven. The 16-bit fixed-point/7-bit final-result compromise lacks a stated scale, signed range, intermediate arithmetic capacity, rounding policy and representation for an invalid or out-of-range height. | Retain separate hump detection and height validity, and update the maximum only after a valid completed encounter. Treat the proposed widths as provisional. Preserve sufficient intermediate precision; packing a final value must not silently truncate, wrap or replace an existing valid maximum. | Recorded: Darrius proposed 16-bit working values and a 7-bit result. Follow-up: define units/range and error budget; check accumulation and overflow. Calibrate the mounted sensor, confirm motion-data continuity and compare repeated steady-speed runs with measured reference humps. | Version A reports terrain logic checks without the IMU and leaves A4/A10 open. Version B's 'few millimeters' criterion is not a confirmed assessment tolerance or measured result. Record actual reference height, speed, repeats and error before accepting accuracy or the packed width. |
| 8. Readiness and resource claims need a traceable acceptance baseline. Version B says the robot is fully integrated but partially ready, while regression and minimum-bit evidence are pending. Green ownership/wiring labels and planned Week 13 evidence do not demonstrate Week 10 readiness. | Record a conditional partial-integration position: documented architecture and preliminary checks support controlled integration, while changed representations and physical behavior remain unvalidated. Retain unchanged ownership/priorities as design decisions, with loaded timing still to be shown. Do not claim RAM savings or complete regression. | Follow-up: all owners link each change to a test, measurable criterion and result before relying on it in the Week 10 demo. Shaun compares RAM/message sizes using identical builds; owners measure packing cost and stack use. Separate R3 stack exhaustion from pointer/bounds errors and track both appropriately. | Version B explicitly leaves risks and tests Open; the telemetry 'success criterion' describes a failure and needs a positive pass condition. Version A records pending physical, timing and ownership clarifications. Zoom timestamps establish decisions only. This review records required work; no new implementation or tests were performed. |

# Final Tribunal Summary

| **Final question** | **Team answer** |
| --- | --- |
| What is the strongest part of the current design? | The strongest part is the separation of authority and data delivery. Motion alone writes motor outputs; Mission owns navigation and accepted configuration changes; Safety retains stop authority; networking does not control movement. Copied sensor snapshots are separate from queued barcode events and retained action results. Front obstacle data is kept separate from side-scan profiles, while hump detection is separated from height validity. These decisions give the minimum-bit redesign a sound boundary: representation may change, but ownership, freshness, validity and acknowledgement semantics must be preserved. This is an architectural strength supported by the descriptions and preliminary checks in Version A, not proof of complete physical performance. |
| What is most likely to fail? | The highest immediate integration risk is incorrect packed-data behavior: undersized ranges, precision loss, rollover or simultaneous writes can produce plausible but wrong status and motion inputs. Version B rates shared-data corruption and interface mismatch High. Barcode-triggered distance resets are a particular cross-subsystem risk because consumers may lose a continuous reference, and a missed barcode defeats the proposed reset interval. During powered operation, stale control data, braking delay, unreliable no-echo interpretation or invalid clearance can turn these errors into unsafe behavior. Encoder accuracy, real barcode capture and hump accuracy are also still open. Their actual failure likelihood cannot be ranked without regression and physical measurements. |
| What evidence supports readiness? | Version A records existing command/feedback/stop logic, inhibited Motion and Mission checks, synthetic line/barcode and obstacle checks, terrain logic checks without the IMU, and inhibited MQTT mapping/reload observations. Version B records agreed redesign directions and identifies owners and risks. Together these support a controlled partial-integration attempt, subject to the outstanding electrical and safety checks. They do not establish completion of the packed representation, measured RAM savings, loaded timing, physical stopping or full robot regression. Readiness should remain conditional until each changed field/interface has a specification, a positive pass criterion and linked results, followed by combined-load and physical trials. Use Zoom timestamps as decision references; attach actual build/log/video evidence separately. |
| What did AI miss that human discussion revealed? | The pre-Zoom AI positions focused on subsystem separation, freshness, stopping and physical validation. The human discussion introduced the minimum-bit constraint and its system-wide consequences: replacing JSON with binary payloads; justifying each field width; preserving independent simultaneous faults; balancing terrain arithmetic precision against compact reporting; quantizing obstacle profiles; and addressing encoder distance when total mission length is unknown. It also retained task priorities and RTOS interfaces rather than redesigning authority. Version C identifies the missing contracts and tests behind those choices; it does not treat the proposed 16-bit/7-bit values or barcode resets as proven solutions. The discussion established requirements and compromises that the earlier AI preparation had not resolved. |
| What did AI correctly identify after discussion? | The post-discussion review supports the earlier warnings that an implemented interface, successful build, synthetic test or functioning dashboard cannot establish powered robot readiness. It also identifies concrete gaps introduced by Version B: absent field ranges/scales and invalid encodings, shared packed-update races, unsafe reset assumptions, undefined binary compatibility, precision/overflow risks, and missing before/after resource measurements. Unchanged priorities do not establish unchanged timing. The recorded Green wiring status needs reconciliation with pending Echo, jumper and supply checks; the 50 ms brake-output target and 'few millimeters' height criterion require measurements and agreed conditions. These are findings from comparing A and B, not independently verified code defects or new test results. Owners must close them with traceable evidence. |

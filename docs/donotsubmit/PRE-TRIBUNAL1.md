# Tribunal 1 preparation — CSC01
## Team information

Group: **CSC01**. Repository: [Autonomous-Robotic-Car](https://github.com/shaun-ie/Autonomous-Robotic-Car).

| Buddy | Student ID | Responsibility / discussion persona |
| --- | --- | --- |
| Shaun | 2500629 | S1 WiFi/MQTT and telemetry — Network Gatekeeper |
| Mahesha | 2503973 | S2 motors, encoders and PID — Real-Time Taskmaster |
| Nicholas | 2501552 | S3 line/barcode; Mission/Safety coordination — Track Purist |
| Darrius | 2500360 | S4 terrain/hump processing — Physics Watchdog |
| Jia Jing | 2502825 | S5 sonar/servo and obstacle profiles — Spatial Optimizer |

## 1.1 Pre-tribunal AI ideation

All entries below are draft preparation answers. Prompts are suggested, not verified as used. Each buddy must review the ideas, record their actual AI discussion and confirm their position before submission.

| Buddy | AI prompt used | Useful AI ideas | Weak / unrealistic AI ideas | Position before Zoom |
| --- | --- | --- | --- | --- |
| Shaun | Suggested: Review our WiFi/MQTT design, stopped mapping updates and network-failure risks. | Keep networking separate from control; acknowledge applied mappings. | Assuming a working dashboard proves robot readiness. | Keep local autonomy independent of WiFi; test reconnect and mapping delivery. |
| Mahesha | Suggested: Review motor ownership, encoder feedback, PID, command expiry and stopping. | One motor writer; time-limited commands; retained completion results. | Assuming zero target speed means stopped, or encoder counts prove ground accuracy. | Keep Motion as the only motor writer; measure accuracy and braking. |
| Nicholas | Suggested: Review line guidance, barcode/junction handling, Mission/Safety and recovery. | Separate line readings from queued barcode events; retain junction context. | Assuming two sensors identify every junction or barcode examples is officially confirmed. | Keep bounded capture and recovery; test real prints, junctions and stop/reset. |
| Darrius | Suggested: Review GY-511 calibration, hump-height estimation and encoder dependencies. | Separate hump detection from height validity; retain the highest valid result. | Treating GY-511 as a gyro or chassis rise as proven hump height. | Validate the proposed height method against measured humps. |
| Jia Jing | Suggested: Review stationary scans, range freshness, missing echoes and bypass clearance. | Confirm stopped wheels; keep forward range separate from side scans. | Treating no echo as clear space or sparse rays as complete clearance. | Require measured geometry and clearance before bypass. |

## 1.2 Week 6 architecture defence snapshot

These seven rows compare proposals with current understanding. They do not claim that W6 was fully understood, internally consistent or physically validated. Reasons below are engineering rationales to discuss; buddy acceptance remains pending.

| Week 6 artifact | What the team claimed in Week 6 | Evidence available now | Still weak or untested | Buddy defender |
| --- | --- | --- | --- | --- |
| Task table / architecture | Safety first, Motion next. W6 also incorrectly labelled single-core as accepted. | Current source uses dual-core micro T-Kernel: application tasks on core 1, networking on core 0. | Loaded timing and cross-core handoffs; resolve conflicting W6 Safety-owner labels. | Nicholas / Mahesha / Shaun |
| Motion statechart | Motion alone drives motors; commands expire and results remain until acknowledged. | Command, feedback, stop and acknowledgement code exists; inhibited logic checks ran. | Wheel PID, ground accuracy and physical braking. | Mahesha |
| Line/barcode statechart | Separate line readings and barcode events; decode outside interrupts; sample every 20 ms. | Current source samples every 10 ms, stores 64 edges and eight events; synthetic firmware checks ran. | Real print decoding, junction context and queue capacity at speed. | Nicholas |
| Communications statechart | Status-only requests; network loss must not block driving. | Stopped runtime mapping was added. Inhibited MQTT mapping changes and reboot/reload were observed. | Full transport stress and reconnect during powered operation. | Shaun / Nicholas |
| Terrain statechart | Only valid complete hump encounters update the maximum height. | Calibration, height-validation and history code exists; logic checks ran without the IMU. | Mounted calibration and reference-hump accuracy. | Darrius |
| Obstacle statechart | Scan while stationary; keep front and profile readings separate. | Scan guards, cancellation and center restoration exist; synthetic checks ran without sonar. | Real range/profile accuracy, clearance and bypass geometry. | Jia Jing |
| Mission statechart | Reset does not start movement; recovery has time/travel limits. | Separate start/reset and guarded recovery exist; inhibited Mission checks ran. | Stop/reset races, finish rules and original-line recovery. | Nicholas |

## 1.3 Shared resource allocation matrix

A resource is shared when multiple tasks, modules or interrupts can affect it. Bounds below are inspected configurations, not proven operating capacity. Exact object bytes, runtime stack margin, peak queue occupancy and interrupt load remain unmeasured here.

| Shared resource | Used by task(s) | Owner task / module | Access type | Contention risk | Mitigation | Evidence needed |
| --- | --- | --- | --- | --- | --- | --- |
| Motors and command state | Mission, Safety, Motion | Motion owns outputs; Mission commands; Safety stops. | Command writes and feedback reads | Conflicting or expired movement requests | One motor writer; stop overrides movement; commands expire. | Physical stop, expiry and reset trials; measure the 50 ms brake-output target. |
| GPIO interrupts | Encoder, barcode, sonar capture | Shared dispatcher; each module owns its pins. | Interrupt capture | Missed edges or long handlers | Encoder-first service; bounded capture; acknowledge owned edges. | Compare counts under simultaneous load; measure handler time and overflow. |
| PWM hardware | Motion, buzzer, servo | Motion slices 4/5; buzzer 3; servo 6. | Owned-slice updates | Resetting PWM disrupts another output. | Separate slices; avoid resetting the shared block. | Check motor, buzzer and servo waveforms together. |
| Motion feedback | Mission, line/barcode, Terrain, Obstacle, reporting | Motion | Copied snapshots | Stale/torn readings or lost completion | Check age/validity; bounded locks; retain results until acknowledged. | Concurrent-reader and lock-timing checks under load. |
| Barcode mapping | MQTT, Mission, line/barcode | S1 transport; Mission acceptance; S3 storage. | Copied request and acknowledgement | Partial updates or replacement while moving | Stopped validation; full 26-entry replacement; four acknowledgement slots. | Busy/backpressure/reconnect tests; inhibited mapping/reload subset already observed. |
| CPU and task memory | Application tasks and network service | Each task owner; shared configuration by team. | Scheduled execution | Missed deadlines or insufficient stack | Bounded work; 4096-byte stacks; Safety/Motion get higher application priority. | Measure execution time, jitter, stack use and queue peaks. |


## 1.4 Subsystem interface and code structure map

All rows are source-level interfaces. Follow their headers for exact types and return values; readers must check validity/age and copy failure. No public API is changed by this document.

| Module / file / interface | Provider | Consumers | Purpose | Inputs | Outputs | Interface rule | Validation rule |
| --- | --- | --- | --- | --- | --- | --- | --- |
| S1 reporting / mapping transport | Shaun | Network service, Mission, laptop | Report state and carry configuration. | Subsystem snapshots; received requests | Telemetry, events, mapping acknowledgement | No movement authority; Mission validates mappings. | Test delivery, invalid requests and reconnect on Pico. |
| Motion command / feedback / stop / acknowledgement | Mahesha | Mission, Safety; feedback readers | Control motors and report motion. | Leased commands, encoder counts, stop state | Motor output, measured motion, action result | Only Motion writes motors; stop/expiry overrides commands. | Measure speed, distance, turns, completion and braking. |
| Line snapshot / barcode event APIs | Nicholas | Mission, reporting | Provide steering guidance and navigation events. | IR readings, edges, encoder travel, mapping | Line snapshot, queued barcode event | No motor writes; preserve sample age and queued events. | Test contrast, prints in both directions, speed and duplicates. |
| Terrain snapshot / calibration APIs | Darrius | Mission, Safety, calibration coordinator, reporting | Classify movement and estimate hump height. | GY-511 samples, motion context | Calibration state, movement, height, maximum | Terrain owns processing; invalid height cannot replace a valid maximum. | Calibrate mounted sensor; compare with known humps. |
| Obstacle snapshot / scan request / cancel | Jia Jing | Mission, reporting | Measure obstacles and clearance. | Echoes, servo settings, scan request, stationary feedback | Front range, scan rays, profile | Scan while stationary; separate front/profile age; Mission owns bypass. | Test known objects, no echo, cancellation and fresh center return. |


## 1.5 Week 6 assumption closure

These are concise summaries of the original assumptions. None is closed without the required evidence. PROJECT_SPECS holds unresolved course and calibration decisions.

| Week 6 assumption ID | Original assumption | Current status | Evidence collected | Remaining risk | Next action |
| --- | --- | --- | --- | --- | --- |
| A1 | WiFi/MQTT recover within the target. | Needs validation | Inhibited mapping/reload and laptop broker checks ran. | Driving reconnect time is unknown. | Shaun: interrupt WiFi/broker and measure recovery and control timing. |
| A2 | Encoder counts and direction remain reliable across speeds and reversals. | Needs validation | Manual wheel trials gave provisional counts; one trial had invalid transitions. | Count errors corrupt motion and barcode widths. | Mahesha: repeat known revolutions at several speeds and directions. |
| A3 | Mounted IR gives clear contrast and complete barcode frames. | Needs validation | Synthetic decoder/capture firmware checks passed. | Lighting, mounting and speed may cause missed/duplicate reads. | Nicholas: record real ADC ranges and barcode edges across conditions. |
| A4 | Pitch and travel can estimate calibrated hump height. | Needs validation | Estimator logic exists; logic checks ran without the IMU. | Slip and acceleration may distort height. | Darrius: compare ordinary and one-wheel climbs with measured humps. |
| A5 | Sonar profiles represent actual obstacles. | Needs validation | Profile logic exists; synthetic checks ran without sonar. | Angled surfaces and missing echoes may imply false clearance. | Jia Jing: compare measured objects, blocked sides and no-echo cases. |
| A6 | Motor/servo load transients stay within electrical limits. | Needs validation | Power policy exists; no accepted load-transient measurements. | Brownout, damage or corrupted sensor readings. | All owners: verify modules and measure logic levels, loaded rails and currents. |
| A7 | Grove wiring and unused contacts match the design. | Needs validation | Shared-pin risks are documented; wiring acceptance is pending. | Shared contacts may conflict with ADC, encoders or battery sensing. | Nicholas/Mahesha: check continuity, unused GP26, GP28 jumper and console pins. |
| A8 | Barcode mapping, orientation and junction offsets are confirmed. | Partly clarified | Four-letter rule confirmed; rehearsal mapping changes and reload observed. | Issued framing, direction, tolerances and spacing remain unresolved. | Nicholas/team: confirm course details and test issued prints. |
| A9 | The course permits bypass and original-line recovery. | Open | Recovery guards exist; no physical bypass pass recorded. | Boundaries or ambiguous lines may invalidate the route. | Jia Jing/team: confirm boundaries; measure geometry; test both bypass sides. |
| A10 | Finish and hump-height assessment criteria are specified. | Open | No confirmed finish convention or accepted height-error limit. | Premature finish or unsupported accuracy claims. | Nicholas/Darrius: confirm rules, references and error limits. |
| A11 | Motion timing and the 50 ms brake-output target hold under peak load. | Needs validation | Schedules and builds exist; worst-case timing is unmeasured. | Delayed control or braking. | All owners: measure execution, jitter, encoder load and stop latency. |
| A12 | Expiry, reset rejection and watchdog recovery prevent unsafe restart. | Needs validation | Stop/reset code and inhibited logic checks exist. | Reset or held-button behavior may allow unsafe continuation. | Nicholas/Mahesha: test expiry, old commands, held buttons, inhibited boot and stalled progress. |
| A13 | Wheel geometry and bounded slip support repeatable motion accuracy. | Needs validation | Geometry/count scaling is provisional; calibration is not accepted. | Encoder rotation may not match ground travel. | Mahesha: set limits, calibrate geometry and repeat externally measured speed/distance/drift/turn trials. |
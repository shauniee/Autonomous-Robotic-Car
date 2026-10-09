# Project specifications

## Required outcomes

The robot must navigate autonomously using onboard sensing and embedded software. Remote driving is outside the mission. The course includes a black line, navigation barcodes, humps and static obstacles.

| ID | Requirement |
| --- | --- |
| M1 | Follow the line; tolerate minor lighting changes, slight imperfections and speed variations |
| M2 | Detect and decode navigation barcodes |
| M3 | Execute left, right, straight and U-turn commands accurately with encoder-based motion control |
| M4 | Detect humps using IMU data |
| M5 | Estimate hump height and report the highest peak experienced during the run |
| M6 | Detect obstacles on or near the line |
| M7 | Profile obstacle shape/position with ultrasonic sensing: location, width, closest point and left/right clearance |
| M8 | Stop or slow, select a safe bypass, avoid collisions and minimize departure from the route |
| M9 | Reacquire the original line after bypass and continue tracking |
| M10 | Report robot/mission telemetry through WiFi and implement MQTT communication |

Buddy 1 also implements command subscription, recovery and heartbeat/status reporting. The proposed design permits validated status requests only; movement stays autonomous. Buddy 3's shorter command list omits U-turn, but M3 retains all four mission commands. Junction detection is in Buddy 3's scope; two-sensor ambiguity needs validated course context.

## Deliverables and assessment

| Area | Deliverables / work |
| --- | --- |
| Communications | Communication API, telemetry framework, topic/message documentation and telemetry demonstration |
| Motion | Motors/encoders, speed/distance estimates, PID wheel-speed tuning, straight correction, repeatable distance/angle actions; control library, tuning report and accuracy evaluation |
| Line / Barcode | Three IR sensors, calibration, line position and junction logic, decoder and navigation interface |
| Terrain | Calibrated IMU processing, tilt/motion/impact/turn-rate reporting, hump algorithm and terrain analysis report |
| Obstacle | Coarse scan followed by focused fine scan; profile, avoidance and original-line recovery algorithms |
| Team | Defined inputs/outputs/owners, repeatable subsystem checks, measured calibration, early integration and integrated demonstration |

Assessment considers mission completion, navigation accuracy, completion time, barcode reliability, hump measurement, obstacle avoidance, reliability/robustness, code quality, resource efficiency, integration and demonstration. No weights or numerical pass marks are supplied. Prepare for a **Week 10 demonstration**; no dated schedule is established here.

Example Motion APIs include forward/backward distance, left/right angle and stop; finalize them through the [Motion contract](architecture/MOTION.md). Reverse/retry is a suggested avoidance option, usable only with proven clearance and permitted course rules. Suggested coarse scan angles are 30°, 60°, 90°, 120° and 150°; they are examples, not calibrated servo endpoints.

## Course geometry

| Feature | Supplied information |
| --- | --- |
| Black line | 18 mm wide |
| Adjacent barcode lane | 20 mm white gap from nearest line edge |
| Illustrated barcode | Approximately 141 mm long in the photographed travel direction |

Verify physical print scale, mounting and whether the illustrated length applies to every symbol before using it as a motion distance.

## Barcode examples

**A → left, B → right, C → straight, D → U-turn** is an assignment example, not a confirmed course mapping. Supplied printed samples are **A and Z**, each at two scales; the larger is about twice the smaller. Both contain 15 black bars in three groups of five. Counting bars cannot distinguish them.

Each sequence alternates black bar / white space, starting and ending with a black bar. `n` = narrow, `w` = wide; inter-character gaps are narrow. Observed wide:narrow ratio is approximately 3:1, not a calibrated tolerance.

| Sample | First group | Middle group | Last group |
| --- | --- | --- | --- |
| A | `nwnnwnwnn` | `wnnnnwnnw` | `nwnnwnwnn` |
| Z | `nwnnwnwnn` | `nwwnwnnnn` | `nwnnwnwnn` |

The patterns are consistent with Code 39 `*A*` and `*Z*`; this is an inference, not a confirmed symbology specification. No final decode table, B/C/D samples, width tolerances, quiet-zone minimum, mounting coordinate, junction offset or traversal direction is established. Use [Barcode](architecture/BARCODE.md) for capture/decode behavior.

## Design targets and team constraints

These are engineering proposals/project decisions, separate from assessment requirements:

- Safety release 10 ms; Motion/Mission/line/Terrain release 20 ms; initial required-data age limit 250 ms; commanded brake within 50 ms. Measure and tune per signal. The implemented bench's stricter limits are in [Line Tracking](architecture/LINE_TRACKING.md#business-logic) and the [handoff](architecture/IR_MOTION_HANDOFF.md#current-interface-contract).
- Network loss must not stop otherwise healthy local autonomy. Status requests cannot override Safety.
- Use calibrated proportional line guidance and separate planned encoder wheel PID. This later decision supersedes the older line-PID proposal; Motion's PID plan remains.
- Adopt [CODING_STANDARD](CODING_STANDARD.md) for application development. GNU C11 and atomic startup publication are retained. A complete Barr compliance audit is not claimed; the supplied assignment text does not establish a mandatory Barr citation.
- Operating power policy belongs to [GETTING_STARTED](GETTING_STARTED.md#power-and-flashing); hardware ratings belong to [TECHNICAL_INFO](TECHNICAL_INFO.md).

## Pending decisions

Record answers and measured values here once, then link from other guides.

| ID | Decision / missing evidence | Owner |
| --- | --- | --- |
| D01 | Final barcode symbology, symbol/action mapping, framing, quiet zones, width tolerances, direction and duplicate rearm condition | S3 / team |
| D02 | Finish, junction/crossing rules, barcode-to-junction spacing, print scale and permitted bypass/reverse boundaries | Team |
| D03 | Motor/encoder ratings and logic levels, decoded counts per wheel revolution, polarity, wheel diameter, track width and car envelope | S2 |
| D04 | Mounted TCRT module voltage/current/output levels, polarity, height, contrast/noise and lighting range | S3 |
| D05 | Actual sonar variant/Echo level, range, repeat interval, missing-echo behavior; servo supply/current, endpoints, center and settling | S5 |
| D06 | IMU supply/pull-ups, axes/bias, filters, impact thresholds and valid height method against reference humps | S4 |
| D07 | Wheel PID, speed/turn accuracy, stall/clearance/stopping thresholds, scan/reacquisition deadlines, retries, leases and queue capacities | Owners / team |
| D08 | Stop/reset serialization, brake acknowledgement, measured stop latency and stationary feedback acceptance | S2 / Safety |
| D09 | Final mission telemetry/status envelope, network/stack load and shared configuration/task-priority approval | S1 / team |
| D10 | Numerical acceptance thresholds, demonstration date, instructor repository access and evidence schedule | Team |

Implementation gaps and project sequence belong to [GETTING_STARTED](GETTING_STARTED.md#project-tasks). Existing T01–T16 IDs remain planned physical/integrated trials; [test records](../testing/README.md) own case definitions and results.

# Project specifications

This file and [CHANGE-REQUIREMENT](CHANGE-REQUIREMENT.md) are the authoritative requirements. The written project brief and the instructor briefing ([project transcript](donotsubmit/ZOOM-TRANSCRIPT-PROJECT-RAW.md), [change-requirement briefing](donotsubmit/CR-BRIEFING-TRANSCRIPT.md)) support them; discrepancies between those sources were resolved with the team on 2026-10-25 and are recorded here. [TRIBUNAL1](TRIBUNAL1.md) records the Week 8 realigned plan. [ARCHITECTURE](ARCHITECTURE.md) owns the design that implements these requirements.

## Required outcomes

The robot navigates autonomously using onboard sensing and embedded software. Remote driving is outside the mission. The course includes a black line, navigation barcodes, humps and static obstacles. The final course map is not disclosed in advance, so no route may be hard-coded.

| ID | Requirement |
| --- | --- |
| M1 | Follow the line; tolerate minor lighting changes, slight imperfections and speed variations |
| M2 | Detect and decode navigation barcodes for all 26 letters A–Z, in either traversal direction |
| M3 | Execute left, right, straight and U-turn commands accurately with encoder-based motion control; turns use precise encoder-measured angles; accuracy holds when one wheel is raised on a hump |
| M4 | Detect humps using IMU data |
| M5 | Estimate hump height; report the number of humps, each completed hump height and the highest peak experienced during the run |
| M6 | Detect obstacles on or near the line |
| M7 | Profile obstacle shape/position with ultrasonic sensing: location, width, closest point and left/right clearance; keep observing while bypassing to measure obstacle depth and detect when it has been passed |
| M8 | Stop or slow, select a safe bypass, avoid collisions and minimise departure from the route |
| M9 | Reacquire the original line after bypass and continue tracking |
| M10 | Report robot/mission telemetry through WiFi using MQTT, with two-way communication (see [Communications](#communications)) |

## Course rules

| Topic | Rule |
| --- | --- |
| Line sensing | Two analogue IR sensors follow the line (GP26/GP27) and a third digital IR sensor reads barcodes (GP16). Three IR sensors is the permitted maximum. |
| Barcode placement | A barcode always precedes a junction. Hold the decoded command and execute it only at the next cross junction. |
| Barcode and hump sides | Barcodes lie on one fixed side of the line and humps on the other fixed side; which side is pending (D11). |
| Humps | Single-wheel humps (not double humps) of varying heights. Measuring while moving scores better than stopping to measure. |
| Obstacles | Static obstacles block the line. Whether they appear only on straight sections is pending (D12). |
| Map | Undisclosed until the demonstration; no hard-coded route. |

### Mission-day barcode rule

Nicholas confirmed on 2026-10-14: the barcode alphabet is A–Z, and the instructor announces exactly four distinct letters on mission day, one each for left, right, straight and U-turn. Firmware must not be recompiled after that announcement. The decoder and control program are finalised and tested beforehand; the four assignments are loaded over WiFi as runtime configuration.

- Firmware boots with every letter unmapped and refuses to start a run until a valid four-letter mapping is loaded.
- The other 22 letters remain unmapped.
- A reboot clears the mapping; the operator resends the same mapping before the next attempt.
- Example assignments are not official. Tests cover the written-brief example (A left, B right, C straight, D U-turn), the briefing example (A straight, B left, C right, D U-turn) and the briefing's demo-day example (A straight, Z U-turn, W left, F right).

## Communications

- Two-way MQTT: the robot publishes telemetry, returns a telemetry snapshot on request and accepts the barcode mapping. Networking never drives, starts or stops the robot.
- On demonstration day the robot connects to the instructor's broker.
- Telemetry covers current robot state, motor speed, encoder counts/distance travelled, line sensor status, faults, barcodes detected with the decoded command, hump count and heights with the maximum, and obstacle width/depth. An event record captures when and where (time and encoder position) barcodes, humps, obstacles and turns/segments occurred, so the approximate route can be reconstructed.
- Payloads are compact binary ([CHANGE-REQUIREMENT](CHANGE-REQUIREMENT.md)). The laptop decoder prints them as readable text; a dashboard is not required.

## Change requirement (minimum-bit data representation)

[CHANGE-REQUIREMENT](CHANGE-REQUIREMENT.md) is change of requirement one, adopted at Tribunal 1 (Week 8). It is implemented in firmware before the final demonstration; the Week 13 submission carries its evidence.

- Every application data field uses only the bits its valid range needs, through C bit-fields or explicit masks and shifts. This covers motion/encoder, line/barcode, obstacle, hump, vehicle/controller state, task-to-task and ISR-to-task data, inter-task messages and MQTT telemetry and command payloads.
- Simultaneous faults (motor stall, encoder fault, IMU fault, no echo, Wi-Fi loss and others) are independent bits in a shared fault bitmask.
- Read-modify-write of packed data shared between tasks, cores or an ISR and a task must be race-free.
- Briefing clarifications: loop counters and temporaries are exempt; small fields may be combined into one integer; each owner justifies every width and provides the accessor functions that read and write packed fields.
- Week 13 evidence in the repository: [data representation table](DATA_REPRESENTATION.md), RAM usage before and after, message size before and after, and evidence of no lost functionality or unacceptable timing degradation.

## Deliverables

| Area | Deliverables / work |
| --- | --- |
| Communications | Communication API, telemetry framework, topic/message documentation and telemetry demonstration |
| Motion | Motors/encoders, speed/distance estimates, PID wheel-speed tuning, straight correction, repeatable distance/angle actions; motion control library, PID tuning report and motion accuracy evaluation |
| Line / Barcode | Three IR sensors, calibration, line position and junction logic, barcode decoder and navigation command interface |
| Terrain | Calibrated IMU processing, tilt detection, motion classification, collision detection, turn-rate monitoring, hump detection/peak measurement, motion event reporting and terrain analysis report |
| Obstacle | Coarse scan followed by focused fine scan (example angles 30°, 60°, 90°, 120°, 150°); obstacle profile generator, avoidance (stop, continue, turn left/right, reverse and reattempt) and original-line recovery |
| Team | Defined inputs/outputs/owners, repeatable subsystem checks, measured calibration, early integration and integrated demonstration |

Turn rate and heading come from encoder differential motion; impact comes from the accelerometer. Magnetometer heading fused with encoders is a stretch goal that the briefing associates with better marks, not a requirement.

## Assessment and demonstration rules

- Assessment: mission completion, navigation accuracy, completion time, barcode reliability, hump measurement, obstacle avoidance, robustness, software design/code quality, resource efficiency, integration and demonstration. Grading is holistic; no weights are supplied.
- Use micro T-Kernel on the Pico; using FreeRTOS is penalised.
- Each owner must explain any line of their submitted code on request; failure attracts a penalty.
- Commits and merges are tracked as contribution evidence; raise team problems with the instructor early.
- Final demonstration: up to three attempts, the most recent counts. Target completion under five minutes; much longer may be penalised. No recompilation after the barcode mapping is announced.
- Requirement changes: the Week 8 change is implemented; the Week 13 change is discussed in writing and need not be implemented.
- Week 10: partial-integration demonstration (format pending, D10).

## Course geometry

| Feature | Supplied information |
| --- | --- |
| Black line | 18 mm wide |
| Adjacent barcode lane | 20 mm white gap from nearest line edge |
| Illustrated barcode | Approximately 141 mm long in the photographed travel direction |

Supplied printed barcode samples and their bar patterns are recorded in [TECHNICAL_INFO](TECHNICAL_INFO.md#barcode-samples). Verify print scale and mounting before using any length as a motion distance.

## Pending decisions

Record answers and measured values here once, then link from other guides.

| ID | Decision / missing evidence | Owner |
| --- | --- | --- |
| D01 | Issued barcode symbology/framing, quiet zones, width tolerances and duplicate rearm | S3 / team |
| D02 | Finish convention, junction/crossing rules, barcode-to-junction spacing, print scale and permitted bypass/reverse boundaries | Team |
| D03 | Motor/encoder ratings and logic levels, decoded counts per wheel revolution, polarity, wheel diameter, track width and car envelope | S2 |
| D04 | Mounted IR module levels, polarity, height, contrast/noise and lighting range | S3 |
| D05 | Installed ultrasonic model and Echo level (≤ 3.3 V before connecting GP2), range, repeat interval and no-echo behaviour; servo supply, endpoints, centre and settling | S5 |
| D06 | IMU supply/pull-ups, axes/bias, filters, impact thresholds and height method against reference humps | S4 |
| D07 | Wheel PID gains, speed/turn accuracy, stall/clearance/stopping thresholds, scan/reacquisition limits and retries | Owners / team |
| D08 | Measured stop latency and stationary-detection threshold | S2 / Safety |
| D09 | Instructor network/broker settings available before program freeze; network/stack load | S1 / team |
| D10 | Numerical acceptance thresholds, Week 10 demonstration format/date, instructor repository access and evidence schedule | Team |
| D11 | Which side of the line holds barcodes and which holds humps | Team |
| D12 | Whether obstacles appear only on straight sections (the briefing states both) | Team |

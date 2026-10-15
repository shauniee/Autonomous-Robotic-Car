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

Buddy 1 also implements command subscription, recovery and heartbeat/status reporting. The implementation accepts read-only status/events requests and stopped barcode configuration; movement stays autonomous. Buddy 3's shorter command list omits U-turn, but M3 retains all four mission commands. Junction detection is in Buddy 3's scope; two-sensor ambiguity needs validated course context.

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

Example Motion APIs include forward/backward distance, left/right angle and stop; finalize them through the [Motion contract](../architecture/SUBSYSTEM2.md). Reverse/retry is a suggested avoidance option, usable only with proven clearance and permitted course rules. Suggested coarse scan angles are 30°, 60°, 90°, 120° and 150°; they are examples, not calibrated servo endpoints.

## Course geometry

| Feature | Supplied information |
| --- | --- |
| Black line | 18 mm wide |
| Adjacent barcode lane | 20 mm white gap from nearest line edge |
| Illustrated barcode | Approximately 141 mm long in the photographed travel direction |

Verify physical print scale, mounting and whether the illustrated length applies to every symbol before using it as a motion distance.

## Mission-day barcode rule

Nicholas confirmed on 2026-10-14: the barcode alphabet is A–Z (26 possible letters), but the instructor announces exactly four distinct letters on mission day, one each for left, right, straight and U-turn. Firmware/program changes are not allowed after that announcement. Finalize and test the decoder/control program beforehand; load the four announced assignments over WiFi as runtime configuration. The other 22 letters remain UNMAPPED. Neither earlier ABCD example is an official assignment.

The existing transport replaces a full 26-entry RAM table, while the laptop operator supplies four assignments. Acknowledgement and fresh local start remain separate. Reboot clears the table, so reload the same announced data without changing firmware. Verify this on the frozen image before the day; [the acceptance procedure](../../GUIDE.md) owns operator steps and evidence.

Issued symbology/printing, physical tolerances and course/network settings must be resolved before program freeze or supported by an already-tested runtime configuration path. Current runtime configuration covers barcode assignments; other compiled settings cannot be left for a late rebuild.

## Barcode examples

**A → left, B → right, C → straight, D → U-turn** is an assignment example, not a confirmed course mapping. Supplied printed samples are **A and Z**, each at two scales; the larger is about twice the smaller. Both contain 15 black bars in three groups of five. Counting bars cannot distinguish them.

Each sequence alternates black bar / white space, starting and ending with a black bar. `n` = narrow, `w` = wide; inter-character gaps are narrow. Observed wide:narrow ratio is approximately 3:1, not a calibrated tolerance.

| Sample | First group | Middle group | Last group |
| --- | --- | --- | --- |
| A | `nwnnwnwnn` | `wnnnnwnnw` | `nwnnwnwnn` |
| Z | `nwnnwnwnn` | `nwwnwnnnn` | `nwnnwnwnn` |

The patterns are consistent with Code 39 `*A*` and `*Z*`; this is an inference, not a confirmed symbology specification. No instructor-confirmed decode specification, B/C/D samples, width tolerances, quiet-zone minimum, mounting coordinate, junction offset or traversal direction is established. Use [Barcode](../architecture/SUBSYSTEM3.md) for capture/decode behavior.

## Design targets and team constraints

These are engineering proposals/project decisions, separate from assessment requirements:

- Safety release 10 ms; Motion/Mission/line/Terrain release 20 ms; initial required-data age limit 250 ms; commanded brake within 50 ms. Measure and tune per signal. [ARCHITECTURE](../ARCHITECTURE.md#safety-and-acceptance) owns the implemented freshness and watchdog defaults.
- Network loss must not stop otherwise healthy local autonomy. Status requests cannot override Safety.
- Use calibrated proportional line guidance and separate encoder wheel PID. This later decision supersedes the older line-PID proposal; Motion's wheel PID remains.
- Adopt [CODING_STANDARD](CODING_STANDARD.md) for application development. GNU C11 and atomic startup publication are retained. A complete Barr compliance audit is not claimed; the supplied assignment text does not establish a mandatory Barr citation.
- Operating power policy belongs to [GUIDE](../../GUIDE.md#power-and-flashing); hardware ratings belong to [TECHNICAL_INFO](../../TECHNICAL_INFO.md).

## Pending decisions

Record answers and measured values here once, then link from other guides.

| ID | Decision / missing evidence | Owner |
| --- | --- | --- |
| D01 | Issued barcode symbology/framing, quiet zones, width tolerances, direction and duplicate rearm; four announced letter identities loaded at runtime under the confirmed rule above | S3 / team |
| D02 | Finish, junction/crossing rules, barcode-to-junction spacing, print scale and permitted bypass/reverse boundaries | Team |
| D03 | Motor/encoder ratings and logic levels, decoded counts per wheel revolution, polarity, wheel diameter, track width and car envelope | S2 |
| D04 | Mounted TCRT module voltage/current/output levels, polarity, height, contrast/noise and lighting range | S3 |
| D05 | Actual sonar variant/Echo level, range, repeat interval, missing-echo behavior; servo supply/current, endpoints, center and settling | S5 |
| D06 | IMU supply/pull-ups, axes/bias, filters, impact thresholds and valid height method against reference humps | S4 |
| D07 | Wheel PID, speed/turn accuracy, stall/clearance/stopping thresholds, measured worst-case braking headroom and sonar mounting offsets, scan/reacquisition deadlines, retries, leases and queue capacities | Owners / team |
| D08 | Stop/reset serialization, brake acknowledgement, measured stop latency and stationary feedback acceptance | S2 / Safety |
| D09 | Final mission telemetry/status envelope, network/stack load, instructor network/broker settings available before program freeze, and shared configuration/task-priority approval | S1 / team |
| D10 | Numerical acceptance thresholds, demonstration date, instructor repository access and evidence schedule | Team |

Project sequence and readiness gates belong to [GUIDE](../../GUIDE.md#acceptance-and-submission). Existing T01–T16 IDs remain physical/integrated acceptance trials, grouped into five UAT passes; [test records](../../testing/README.md) own case definitions and results.

## Requirements traceability

| Requirement | Current verification owners |
| --- | --- |
| M1 line tolerance | T03/T11/T14; S3 and integration |
| M2 A-Z decoding | S3-U09/U15/U16; T04 both scales/orientations |
| M3 four encoder actions | S2-U07/U08/U10/U11; INT-16/21/22; T05 |
| M4/M5 humps and greatest valid height | S4-U03-U10; T06/T07/T14 and reference measurements |
| M6/M7 detection/profile | S5-U04-U06; T08/T09 |
| M8/M9 bypass/original route | INT-23; T10/T11 and confirmed depth/boundaries |
| M10 reporting/MQTT | S1-U04/U06-U13; INT-18; T12/T13/T15 |
| Combined black/IMU calibration | S3-U14/U17; S4-U05/U06/U09/U10; T01/T02 |
| Required health and shared-lock scheduling | INT-24/25; T09/T14/T15 and measured stop latency |

[Testing](../../testing/README.md) owns definitions/results. [GUIDE](../../GUIDE.md#acceptance-and-submission) owns the release checklist. No firmware fixture/build substitutes for measured mission acceptance or completed analysis reports.

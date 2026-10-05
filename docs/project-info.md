# Autonomous Robotic Car Challenge — project information

## Source and status

The [CSC01 Design Review](<../../CSC01 Design Review.docx>) is the primary integrated design reference. This file preserves the assignment source requirements. The review assigns Nicholas integration/Safety/Mission/line, specifies local GP20/GP21 controls, status-only MQTT and healthy autonomy through network loss. Proposed FreeRTOS tasks and T01–T16 trials are described in [architecture](ARCHITECTURE.md); they are not implemented/tested merely because the review lists them. Current bench calibration commands and swapped IR ports are explicit later implementation differences.

This file contains information about the assignment. It describes required behavior and team deliverables. The technical PDFs, candidate wiring, and component limits are in [tech-info.md](tech-info.md).

## Project purpose and operating setting

The team must design, build, integrate, test, and demonstrate an autonomous robotic car that travels a predefined course without remote driving. The car is expected to sense its surroundings, choose actions, move, and report its status as one dependable embedded system. The course includes a black line to follow, directional barcodes at selected locations, speed humps of varying heights, and static obstacles on or near the line. The assignment emphasizes embedded design, sensor integration, resource-efficient programming, real-time decisions, autonomous navigation, hardware/software co-design, clean interfaces, repeated subsystem tests, early integration, calibration evidence, and a dependable final demonstration.


## Mission requirements

| ID | Required outcome | What the write-up says |
| --- | --- | --- |
| M1 | Follow the track | Follow the black line autonomously using IR sensing; tolerate minor lighting changes, small track imperfections, and speed changes. |
| M2 | Read navigation barcodes | Detect and decode barcodes placed at selected course locations. |
| M3 | Act on barcode commands | Execute **left**, **right**, **straight**, and **U-turn** commands accurately using encoder-based motion control. |
| M4 | Detect humps | Recognize one or more humps from IMU data. |
| M5 | Measure peak hump | Estimate hump height using IMU data and report the highest hump peak experienced during the run. |
| M6 | Detect obstacles | Detect objects on or near the line. |
| M7 | Profile obstacles | Use ultrasonic sensing to estimate obstacle shape/position, width, closest point, and left/right clearance. |
| M8 | Bypass safely | Stop or slow, select a safe maneuver, avoid collision, and minimize departure from the intended route. |
| M9 | Recover route | Find and reacquire the original line after bypassing the obstacle, then continue normal navigation. |
| M10 | Report telemetry | Send robot and mission data over WiFi; the buddy brief specifically calls for MQTT communication and command subscription. |

### Expected mission flow

1. Start an autonomous run with the robot ready to sense and move. The source does not define the exact start trigger.
2. Track the black line continuously. Detect any adjacent or crossing barcode and turn it into a navigation command.
3. Execute the command through controlled motor movement and encoder feedback, then resume line tracking.
4. While traversing the course, detect humps and keep the maximum valid peak measurement for reporting.
5. On obstacle detection, slow or stop, scan, estimate available clearance, choose a bypass or reverse/retry action, and avoid contact.
6. Search for the original line after a bypass and return control to normal line following.
7. Send meaningful status and measurements through WiFi during the run and demonstrate the integrated outcome.

This sequence summarizes the stated behaviors; exact state transitions, timeout values, stop rules, retry limits, and end-of-course behavior still need team decisions.

## Five subsystems and ownership

The briefing describes five subsystems plus a central vehicle controller that handles mission state and coordination. All members also participate in integration, testing, debugging, and the final demonstration, and should understand the shared interfaces. The integrated car is what is assessed.

### Shaun — WiFi communication, commands, and telemetry

**Primary hardware:** WiFi. **Responsibilities:** connect to WiFi; establish MQTT; design topic names and message structures; publish telemetry; subscribe to commands; recover from disconnections; report heartbeat/status. Candidate telemetry fields named by the brief are current robot state, motor speed, encoder counts, line sensor status, detected barcode, hump measurements, obstacle scan results, and distance travelled. **Deliverables:** communication API, telemetry framework, MQTT documentation, and a telemetry demonstration.

The PDFs do not choose a broker, network, authentication method, message format, topic names, reconnect policy, or required behavior when WiFi drops. They do not authorize remote manual driving; command subscription is a subsystem requirement within an autonomous mission.

### Mahesha — motor and encoder motion control

**Primary hardware:** motors and wheel encoders. **Responsibilities:** integrate the motor driver and encoders; estimate speed and distance; tune PID speed control; correct straight travel; execute repeatable encoder-based turns; calibrate motion. The write-up gives example reusable APIs: `moveForward(distance)`, `moveBackward(distance)`, `turnLeft(angle)`, `turnRight(angle)`, and `stop()`. **Desired performance:** consistent speed, accurate travel distance, repeatable turns, and stable motion. **Deliverables:** motion-control library, PID tuning report, and motion accuracy evaluation.

The documents give no wheel diameter, gear ratio, encoder counts per revolution, wheelbase, speed target, turn tolerance, or PID acceptance criteria. These need measurement or agreement before calibration can be judged.

### Nicholas — IR line following and barcode navigation

**Primary hardware:** three IR sensors. **Responsibilities:** integrate and calibrate sensors; estimate line position; follow the line; recognize junctions; detect and decode barcodes; produce navigation commands. **Desired performance:** reliable line tracking and barcode recognition over varying speeds. **Deliverables:** line-following module, barcode decoder, and navigation-command interface.

The system-level list requires four commands, including U-turn. The Buddy 3 task list explicitly lists left, right, and straight but omits U-turn. Treat the system-level four-command requirement as in scope until the instructor says otherwise. The write-up's A/B/C/D mapping is introduced as an **example**, not as a specified barcode symbology or a measured encoding: A → left, B → right, C → straight, D → U-turn.

### Darrius — IMU motion and terrain monitoring

**Primary hardware:** IMU. **Responsibilities:** integrate and calibrate it; detect tilt and humps; measure peak hump height; classify movement; detect collisions; monitor turn rate; report events. The sample event names are stationary, accelerating, turning, climbing a hump, descending a hump, and sudden impact. **Desired performance:** reliable hump detection, accurate peak measurement, and stable event classification. **Deliverables:** IMU processing module, hump-detection algorithm, and terrain analysis report.

The assignment does not define how an IMU reading becomes a physical hump height, how height accuracy is scored, or which reference measurements to use. The supplied component's actual sensing limits are discussed in [tech-info.md](tech-info.md).

### Jia Jing — servo ultrasonic obstacle scanning and recovery

**Primary hardware:** ultrasonic sensor on a servo. **Responsibilities and proposed stages:**

1. Take a coarse scan at a small set of angles; the write-up offers **30°, 60°, 90°, 120°, 150° as examples**, not mandatory angles.
2. If an obstacle is found, rescan its region at finer angular resolution.
3. Estimate its location, width, closest point, and clearance on both sides.
4. Propose stop, continue, left, right, or reverse/retry behavior.
5. Search for and reacquire the original line, then hand control back to line following.

**Desired performance:** accurate detection and profiling, effective avoidance, and reliable line reacquisition. **Deliverables:** scanning subsystem, obstacle profile generator, avoidance algorithm, and recovery algorithm.

The course dimensions, obstacle sizes/shapes, minimum clearance, servo sweep time, and allowable bypass area are not specified.

## Suggested architecture and integration contract

The write-up suggests three layers: **sensors** (IR, encoders, IMU, ultrasonic, barcode sensing), **control** (motion, line following, obstacle avoidance, navigation logic), and **communication** (WiFi, MQTT, telemetry).

## Assessment and evidence

The write-up lists mission completion, navigation accuracy, obstacle avoidance, barcode reliability, hump measurement, robustness, software design, resource efficiency, integration, and demonstration performance. The briefing groups assessment under mission, navigation, perception, robustness, engineering, integration, and demonstration. No weights or numerical pass marks are provided.

The briefing explicitly calls for: defined inputs/outputs/owners; repeatable subsystem tests; recorded measurements and calibration; early integration using stable APIs; and practice for a **Week 10 demonstration**. It does not supply a dated schedule or a Week 10 marking sheet.
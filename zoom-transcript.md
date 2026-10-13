# Zoom briefing: consolidated mission and requirements

Source: [formatted project briefing transcript](zoom-transcript-raw.md), speaker Muhamed Fauzi Bin Abbas. The export records 31 August 2026 and a duration of 35 minutes.

This is a source summary, not a verbatim transcript, implementation plan or record of completed work. It consolidates the professor's spoken expectations for the default autonomous-car project. The source was checked against supplied text, not the recording; recognition errors remain possible. Requirements, assessment preferences, advice and unresolved details are distinguished below.

The consolidated [project specifications](../code/docs/PROJECT_SPECS.md) remain the current project authority. This document preserves briefing evidence without changing subsystem ownership, current bench decisions or implemented capabilities.

## 1. Overall project

Design, develop, integrate and test an **autonomous robotic car that completes a predefined course**.

- Collect sensory information, decide what to do and act using onboard embedded software.
- Complete the mission without remote driving.
- Combine line following, directional barcode navigation, hump measurement, static-obstacle avoidance and Wi-Fi/MQTT telemetry into one demonstrable system.
- Demonstrate reliable behaviour across repeated runs, rather than isolated working functions.
- Use **Micro-T-Kernel on Raspberry Pi Pico**. The professor stated that using FreeRTOS instead would attract a penalty; no penalty amount was supplied.

Source sections: [autonomous mission](zoom-transcript-raw.md#welcome-and-autonomous-mission), [RTOS](zoom-transcript-raw.md#required-real-time-operating-system).

## 2. Complete mission behaviour

The course contains a line, barcodes before cross junctions, humps of different heights and static obstacles. The table describes responses to encounters, **not a fixed order of course features**.

| Encounter | Required behaviour |
| --- | --- |
| Ordinary line | Follow the line continuously and autonomously. |
| Barcode | Read and decode its symbol in either traversal direction; retain its configured navigation command. |
| Associated cross junction | Execute straight, left, right or U-turn at the junction. Do not turn immediately upon reading the barcode. |
| Hump | Detect it, estimate its height and report the measurement. Coordinate motion and line following while one wheel is raised. |
| Obstacle blocking the line | Detect it, scan left/right using servo-mounted ultrasonic sensing, estimate its width and available clearance, and choose a suitable bypass side. |
| Bypass | Leave the line, travel around the obstacle without line guidance, and continue observing sufficiently to determine when it has been passed. |
| After bypass | Rejoin the original line and resume tracking. |
| Throughout the run | Publish useful movement, barcode, hump and obstacle telemetry through MQTT; respond to a requested current snapshot/summary. |

The transcript does not define the finish marker, end-of-mission stopping condition, feature counts, complete route or the handling of multiple barcodes before a junction.

Source sections: [autonomous mission](zoom-transcript-raw.md#welcome-and-autonomous-mission), [junction commands](zoom-transcript-raw.md#mission-assessment-and-configurable-barcode-commands), [obstacle bypass](zoom-transcript-raw.md#subsystem-5-ultrasonic-scanning-and-obstacle-avoidance).

## 3. Subsystem requirements

### Subsystem 1: Wi-Fi, MQTT and telemetry

- Use MQTT to publish useful car information to a broker and PC subscriber.
- Support two-way communication: transmit information and receive requests/configuration.
- Return a snapshot or summary of current telemetry upon request.
- Support Wi-Fi changes to barcode-to-action mappings without recompilation.
- Connect to the instructor's broker during the final demonstration; a laptop broker is for development.
- Make received messages understandable. A fancy display is not required.

Telemetry described by the professor includes:

- Car movement, including straight/left/right travel and distance information.
- Number of humps detected and their heights.
- Barcodes encountered, decoded messages and associated commands.
- Obstacle measurements, including width.
- Where or when obstacles and barcodes were encountered, together with movement history sufficient to understand the approximate route.

The professor described building an approximate picture of the route from movement/events. No map visualisation, coordinate system, localisation algorithm or message schema was prescribed. Two-way configuration does not imply permission for remote driving.

Source sections: [Wi-Fi and telemetry](zoom-transcript-raw.md#subsystem-1-wi-fi-mqtt-and-telemetry), [mapping changes](zoom-transcript-raw.md#mission-assessment-and-configurable-barcode-commands), [navigation telemetry](zoom-transcript-raw.md#navigation-telemetry-and-changing-barcode-mappings).

### Subsystem 2: Motion control and PID

- Implement motor drivers.
- Use PID to regulate wheel/motor speed and address differences between the two motors; equal PWM alone does not guarantee equal speed.
- Tune PID for the actual car. Values are not assumed transferable between groups.
- Achieve accurate straight motion and controlled turns at specified angles; 45° and 90° were examples.
- Coordinate line following when one wheel climbs a hump.
- Provide accurate motion for obstacle bypass and line reacquisition.

**Advice:** Finalise the wheels/mechanical setup before tuning PID.

Source: [motion control and PID](zoom-transcript-raw.md#subsystem-2-motion-control-and-pid).

### Subsystem 3: Line following and barcode decoding

- The professor permits two IR sensors for line following and a third for barcode reading. Exact mounting placement is left to the team.
- Decode the barcode regardless of traversal direction.
- Support the full **A–Z alphabet**, not only four selected command letters.
- Retain the decoded navigation command and apply it at the subsequent cross junction.
- Support straight, left, right and U-turn.
- Accept a changed symbol-to-action mapping through Wi-Fi on demo day, without recompiling the code.

The transcript gives this example default:

| Symbol | Transcript example action |
| --- | --- |
| A | Straight |
| B | Left |
| C | Right |
| D | U-turn |

This is **not the final demo mapping**. The written assignment example recorded in [PROJECT_SPECS](../code/docs/PROJECT_SPECS.md#barcode-examples) differs: A → left, B → right, C → straight, D → U-turn. Preserve both as source examples; neither establishes the final mapping.

Source sections: [IR and barcode decoding](zoom-transcript-raw.md#subsystem-3-line-following-and-barcode-decoding), [junction commands](zoom-transcript-raw.md#mission-assessment-and-configurable-barcode-commands), [A–Z and no recompilation](zoom-transcript-raw.md#navigation-telemetry-and-changing-barcode-mappings).

### Subsystem 4: IMU and hump measurement

- Implement the GY-511 driver.
- Use acceleration and magnetic information for useful orientation/direction observations. The professor corrected his initial gyroscope description to **accelerometer and magnetometer**.
- Detect humps and estimate their heights.
- Calibrate readings against known physical heights/tilts, then derive and tune an estimation method.
- Coordinate with Motion so travel permits useful measurements; slowing down was suggested.

**Assessment preferences:** Combine IMU and encoder information to improve accuracy/robustness. Measuring while moving was described as earning better marks than repeatedly stopping to measure; stopping was left as an available design choice.

No height formula, numerical tolerance, sensor-fusion algorithm or calibrated threshold was specified.

Source: [IMU and hump measurement](zoom-transcript-raw.md#subsystem-4-imu-and-hump-measurement).

### Subsystem 5: Ultrasonic scanning and obstacle avoidance

- Use a servo-mounted ultrasonic sensor to scan left and right.
- Estimate obstacle width and available gaps; select a suitable bypass side.
- Allow sufficient clearance for the car's full width.
- Continue observing the obstacle to determine when it has been passed; depth observation was described.
- Leave the line to bypass the obstacle, then rejoin the original line.
- Report obstacle width through Wi-Fi telemetry.

Source: [ultrasonic scanning and obstacle avoidance](zoom-transcript-raw.md#subsystem-5-ultrasonic-scanning-and-obstacle-avoidance).

## 4. Course constraints stated verbally

| Feature | Briefing statement |
| --- | --- |
| Barcode/junction order | The barcode occurs before its cross junction; act at the junction. |
| Hump | Only one wheel traverses the hump. |
| Hump/barcode sides | Opposite sides; their fixed left/right arrangement was to be confirmed when equipment was issued. |
| Obstacle placement | Earlier passages explicitly say a straight section, not a curve. One later sentence says both straight and curved; confirm placement. |
| Final map | Undetermined at briefing time and described as a surprise for the final day. |

These statements do not establish measured course geometry, junction spacing, bypass boundaries or the eventual issued layout.

Source sections: [mission](zoom-transcript-raw.md#welcome-and-autonomous-mission), [obstacle/course placement](zoom-transcript-raw.md#subsystem-5-ultrasonic-scanning-and-obstacle-avoidance), [junction sequence](zoom-transcript-raw.md#mission-assessment-and-configurable-barcode-commands).

## 5. Final demonstration and assessment

### Assessment considerations

- Mission completion and navigation accuracy.
- Correct barcode decoding and command execution.
- Hump-height measurement and obstacle avoidance.
- Completion time and reasonable travel speed.
- Reliability/robustness across runs.
- Code quality, efficiency and integration of the complete system.
- Improved accuracy/robustness through combined sensing, and hump measurement while moving, as described above.

### Demo rules stated verbally

- Up to **three attempts** are allowed.
- The **most recent attempt's result counts**, even if worse than an earlier attempt.
- The team can choose to stop after an earlier attempt.
- A well-performing run should complete in approximately **five minutes**. Excessive time may attract a penalty; no numerical driving speed was mandated.
- Selected barcode command letters change on demo day.
- Recompilation after receiving the mapping will not be allowed; change the mapping through Wi-Fi.
- Use the instructor's MQTT broker.

No exact score weights, penalty amounts or hard five-minute cutoff were supplied. The written-source Week 10 demonstration reference belongs to [PROJECT_SPECS](../code/docs/PROJECT_SPECS.md#deliverables-and-assessment); it is not added here as a transcript statement.

Source sections: [mission assessment](zoom-transcript-raw.md#mission-assessment-and-configurable-barcode-commands), [completion time and attempts](zoom-transcript-raw.md#completion-time-and-demo-attempts), [mapping changes](zoom-transcript-raw.md#navigation-telemetry-and-changing-barcode-mappings).

## 6. Development, ownership and individual understanding

- The default project has five members, each owning one subsystem: communications, motion, line/barcode, terrain and obstacle.
- Develop drivers and subsystem behaviour, then integrate progressively, including pairs or small groups before combining the complete system.
- Test rigorously for repeatable behaviour.
- AI-generated code is permitted, but each owner must understand and explain every submitted line within their subsystem.
- Inability to explain a queried line can attract a penalty; no penalty amount was given.
- The professor stated that the Pico is capable of handling the project and that a basic Micro-T-Kernel repository would be shared.
- Ask for help through discussion groups/forums or face-to-face when needed.

These are briefing expectations and advice, not proof of current implementation or hardware success.

Source sections: [ownership](zoom-transcript-raw.md#team-size-and-subsystem-ownership), [code understanding](zoom-transcript-raw.md#ai-generated-code-and-individual-understanding), [integration advice](zoom-transcript-raw.md#development-and-integration-advice), [RTOS support](zoom-transcript-raw.md#required-real-time-operating-system).

## 7. Group grades and contribution evidence

- The professor first grades the project holistically.
- Peer evaluation and contribution evidence can raise or lower individual grades for the group-work component.
- GitHub commits and merges are used as contribution evidence.
- Commit and push regularly with meaningful descriptions rather than leaving all uploads until the end.
- Provide evidence for contribution claims during peer evaluation.
- Report serious contribution problems early. Anonymous reporting may be possible; do not wait until the final one or two weeks.

The illustrative grade changes in the transcript are examples, not a fixed adjustment formula.

Source: [group grades, peer evaluation and contribution evidence](zoom-transcript-raw.md#group-grades-peer-evaluation-and-contribution-evidence).

## 8. Requirements changes and tribunals

| Milestone | Spoken expectation |
| --- | --- |
| First tribunal / Week 8 | Incorporate the supplied requirements change into the project. |
| Later change / Week 13 | Discuss how to implement the change and put the design/implementation implications in writing; implementation is not required. |

The professor described these exercises as training students to use AI to identify challenges and pivot the design when requirements change. The actual changes were not supplied in this transcript.

Source: [requirements changes and tribunals](zoom-transcript-raw.md#requirements-changes-and-tribunals).

## 9. Unresolved details and current-guide differences

This section records evidence boundaries, not a second pending-decisions register. Confirmed project decisions and measured values belong in the single [PROJECT_SPECS pending-decisions table](../code/docs/PROJECT_SPECS.md#pending-decisions).

The transcript does not settle:

- Final barcode symbology, framing, width tolerances or quiet zones.
- Demo-day mappings, barcode-to-junction distance or handling of multiple pending commands.
- Finish detection, end-of-mission stopping, feature counts/order or complete route.
- Actual fixed hump/barcode sides and the contradictory obstacle-placement sentence.
- Obstacle dimensions, permitted bypass/reverse area or numerical clearance limits.
- Accuracy tolerances for motion, height and obstacle-width estimates.
- Exact time penalties, assessment weights or individual adjustment formula.
- Demo broker address, credentials, topics or payload schema.
- Requirements-change contents and dated assessment schedule.

The current [Communications guide](../code/docs/architecture/COMMUNICATIONS.md#functionality) permits status-only requests, whereas the transcript requires Wi-Fi barcode-mapping configuration. This is a documented gap affecting S1 and S3, not authorization to change either subsystem. The mapping-example discrepancy is recorded in Section 3. Current capability and physical-validation claims remain owned by [GETTING_STARTED](../code/docs/GETTING_STARTED.md#current-capabilities) and [testing](../code/testing/README.md).

## 10. Alternative-project advice

The requirements above concern the default robotic-car project. For alternative projects, the professor described the published descriptions as general starting points with room to pivot. Interested students should read the description, research it, and approach him with specific questions and the description available on their laptop, or ask through the discussion forum.

Source: [alternative projects and closing](zoom-transcript-raw.md#alternative-projects-and-closing).

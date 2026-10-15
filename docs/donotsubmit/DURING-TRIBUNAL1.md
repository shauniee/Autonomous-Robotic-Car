# Information about tribunal 1

Architecture Defence & Integration Readiness | AI ideation, live update, AI-vetted gap check
The core objective of this tribunal is to assess the engineering reasoning and see how the team responds to a sudden change in embedded system requirements.

# Breakdown
- Live Zoom-updated version. Design decisions are updated during the Zoom discussion through challenge, negotiation, and constraint response.
> Marks are not awarded for blindly following AI. Marks are awarded for critical evaluation and human engineering judgement
> Team is graded based on their discussion during zoom

| Item | Details |
|---|---|
| Purpose | Defend Week 6 architecture, justify changes, and show readiness for partial integration |
| Core workflow | Pre-submission AI ideation -> live Zoom-updated version -> final AI-vetted gap check |
| Assessment focus |Evidence, engineering judgement, negotiation, and ability to identify missing work |
| Submission format	| Completed document plus Zoom recording and evidence links| 

| Buddy | Primary subsystem / persona |
|---|---|
| Shaun | Network Gatekeeper: WiFi, MQTT, telemetry |
| Mahesha | Real-Time Taskmaster: motors, encoders, PID |
| Nicholas | Track Purist: IR sensors, line following, barcode |
| Darrius | Physics Watchdog: IMU, terrain, hump/motion events |
| Jia Jing | Spatial Optimizer: ultrasonic, servo, obstacle profiling |

# Requirements

- A zoom session will be done by the professor, detailling the specific changes in the project requirements
- Following the briefing, the team will need to discuss among themselves.
- It will be a technical discussion not coding session, where the team must discuss the engineering impact of the new requirement on the system's resources, memory, processing time, hardware constraints, and module dependencies.
- Every member must actively explain how the requirement change affects their specific sub-module. (at least one challenge, defence, trade-off, or correction)
- Dicussion is to be noted down.
- Changes from version A needs to be recorded and why. Changes must have strong justification
- If AI-generated content is introduced, member needs to explain the prompt used, output received and why the output is being considered.
- AI suggestions must be challenged, validated, refined, or rejected through human engineering discussion; “AI said so” is not sufficient justification.
> Acceptable evidence examples: GitHub commit link, short test log, video timestamp, oscilloscope or serial monitor screenshot, pin test result, task timing measurement, AI prompt and critique, meeting decision note, or photo of hardware wiring.
> AI-use declaration: Do not ask AI to complete this template on behalf of the team. Use AI as a reviewer, challenger, or brainstorming partner. The team confirms that all final entries were reviewed, corrected, and accepted by the team, and that every member can explain the decisions related to their subsystem.

## Buddy contributions during zoom

Each buddy must complete the table. The contribution must be a real technical challenge, defence, trade-off, or correction raised during Zoom, not only an attendance statement.

| Buddy | Key Challenge, Concern, or Argument Raised | Challenge Recevied | Decision or Compromise |
| --- | --- | --- | --- |

## Live Decision Change Log

Enter at least 3 changes AND 3 confirmed no-change decisions from the Zoom discussion. Include the change type and evidence link or timestamp whenever possible. One row may state “No change” if the team defended the Week 6 design successfully.

| Original idea / Week 6 design | Updated decision | Change type | Reason for change | Evidence link / timestamp | Owner / follow-up |
| --- | --- | --- | --- | --- | --- |
|  |  | No change / Clarified / Simplified / Reassigned / Deferred / Rejected / Major redesign |  |  |  |

## Resource and Assumption Review

| Item Challenged | Evidence presented | Decision made | Impact on other buddies |
| --- | --- | --- | --- |

## Risk Assessment and Subsystem Test Plan

### Integration Risk Register

Enter at least 3 integration risks. Include at least one timing or scheduling risk, one hardware or wiring risk, and one interface or team-integration risk. 

| Risk | Cause | Impact | Likelihood | Severity | Mitigation | Owner | Status |
| --- | --- | --- | --- | --- | --- | --- | --- |

### Subsystem Test Plan

Enter at least 4 cross-subsystem tests. Each test must involve the work of at least two buddies and must include a success criterion and evidence type, such as a video, serial log, screenshot, timing result, or commit link. At least TWO test should involve timing, safety, or shared-resource concern. 

Example:
| Subsystem | Test Case | Success Criteria | Evidence Required | Owner | Status |
| Example: sensor input	| Detect line, barcode, or obstacle. | Correct reading appears in serial log | Screenshot or log | Buddy 3 | Open |

## Constraint Injection / Curveball Response

- Record the injected constraint exactly as given.
- Explain how it affects all five buddies, even if the impact differs by subsystem.
- A good response shows prioritisation, trade-off reasoning, fallback behaviour, and evidence-aware decision making.

| Injected constraint | Buddy affected | Immediate Impact | Adaptation / Compromise |
| --- | --- | --- | --- |

## Integration Readiness Self-Assessment

Green only if there is evidence, Yellow if partly ready or untested, and Red if the area is unclear or blocking Week 10 integration.

| Readiness area | Status | Evidence | Concern |
| --- | --- | --- | --- |
| Hardware wiring known | | | |
| RTOS task ownership clear | | | |
| Shared resources identified | | | |
| Critical statecharts implementable | | | |
| Safety or fault handling defined | | | |
| Week 10 partial integration feasible | | | |

## Updated readiness position before Week 10 Partial integration demo.
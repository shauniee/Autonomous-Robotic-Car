# Information about tribunal 1

Architecture Defence & Integration Readiness | AI ideation, live update, AI-vetted gap check
The core objective of this tribunal is to assess the engineering reasoning and see how the team responds to a sudden change in embedded system requirements.


## Quick guide — what you are doing

Your job is to explain how the new requirement affects the robot, compare practical responses and agree what must change. This is a technical discussion, not a coding session. The change has not been announced; the points below are preparation, not live findings or team agreement.

### What to know before the change

| Area | What we need to understand |
| --- | --- |
| Original plan | W6 records our initial ideas, including mistakes and untested assumptions. Explain why a proposal made sense and what evidence could change it. |
| Current architecture | Use dual-core micro T-Kernel: networking on core 0, application tasks on core 1. W6's single-core wording is an initial-plan error.  |
| Ownership | Motion alone writes motor outputs. Mission requests actions; Safety can inhibit movement. Sensors provide observations, not direct motor commands. |
| Data flow | Know each module's inputs, outputs, units, validity and sample age. Identify who depends on the data and what happens when it is missing or late. |
| Timing and capacity | Current configured periods: Safety/line 10 ms; Motion/Mission/Terrain 20 ms; reporting cache 20 ms; telemetry 500 ms. Barcode storage: 64 edges and eight events. These are configured values, not proven timing or load margins. |
| Evidence | Builds and inhibited Pico logic checks exist. Stopped MQTT mapping/reload was observed. Powered motion, braking, real barcode reading, hump accuracy and bypass acceptance remain pending. |
| Course uncertainties | Barcode framing/spacing, junction/finish rules, bypass boundaries and numerical acceptance limits remain unresolved. Do not invent values to make a solution appear complete. |

### Everyone's focus

| Buddy | What to explain | What to challenge or negotiate |
| --- | --- | --- |
| Shaun — communications | Telemetry, request validation, mapping acknowledgement and reconnect. | Can extra messages or retries delay control? Agree data age and mapping acknowledgement with Nicholas. |
| Mahesha — motion | Encoders, wheel PID, bounded actions, completion and stopping. | Can the real car meet the new speed/accuracy demand? Agree stationary feedback and command expiry with Nicholas/Jia Jing. |
| Nicholas — line/barcode and Mission/Safety | Line guidance, retained barcode command, junction action and stop/reset. | Can decoding keep up, and can the correct junction/route be identified? Agree encoder travel, mapping revision and obstacle interruption handling. |
| Darrius — terrain/humps | IMU calibration, movement classification and highest valid hump height. | Does changed speed or motion invalidate height estimates? Agree useful travel/speed data with Mahesha. |
| Jia Jing — obstacle sensing | Stationary scans, obstacle width, clearance and fresh forward range. | Is there enough stopping/scan time and space for the whole car? Agree braking feedback and bypass/recovery limits. |

### How to tackle the announced change

| Step | What to do | What to write down |
| --- | --- | --- |
| 1. Capture the change | Record the professor's exact wording. Ask about ambiguous limits, units, conditions and expected outcomes. | Old requirement, new requirement and clarification still needed. |
| 2. Trace the effects | Each buddy explains direct effects and dependencies on other modules. Follow sensor input through processing, decision, action and telemetry. | Affected modules and who must coordinate. |
| 3. Check feasibility | Compare required rates, memory, processing time and hardware capability with available capacity. Include safety and shared-resource conflicts. | Known values, calculations and missing measurements. |
| 4. Compare options | Discuss at least two feasible approaches where alternatives exist. Challenge AI suggestions and explain benefits, costs and limitations. | Selected/rejected options and reasons. |
| 5. Agree the response | Decide what changes, what stays and what must wait for clarification/testing. Give every action an owner. | Decision, trade-off, dependent interface changes and follow-up owner. |
| 6. Plan verification | Define cross-subsystem tests with a clear success condition and evidence to collect. Set unknown limits before testing. | Setup, expected result, measurement/evidence and owner. |

### Checks to make before accepting an answer

| Check | Question to ask |
| --- | --- |
| Processing and timing | Can the task finish before its next required update? What happens under simultaneous capture, scans and network retries? |
| Memory and queues | How many extra records/bytes are needed? Can bursts fill the buffer before the consumer catches up? |
| Shared resources | Do tasks compete for GPIO interrupts, PWM, locks or copied state? Could a delayed owner block a more urgent task? |
| Interfaces and dependencies | Do units, validity, message size, sample rate or completion rules change? Which consumers must adapt? |
| Hardware and safety | Can the installed sensors/actuators meet the demand? What happens on stale data, no echo, command expiry or failed recovery? |
| No-change claim | Why can the existing design meet the new demand? Show capacity and interface reasoning; mark unmeasured margins as pending. |

**Hypothetical example — higher travel speed:** barcode edges arrive faster, there is less time before the junction, and stopping distance may increase. Nicholas checks capture/service capacity and junction context; Mahesha checks tracking and braking; Jia Jing checks detection distance and scan timing; Darrius checks height accuracy; Shaun checks reporting load. Compare faster service, less noncritical work or another permitted approach. Do not assume the hardware can meet the new speed.

Useful calculations: update frequency = 1000 / period in milliseconds; task CPU share ≈ execution time / period; extra buffer memory = extra records × bytes per record; travel during delay = speed × delay. Use consistent units and actual measurements where available. These estimates alone do not prove schedulability or physical stopping distance.

### Recording and completing the live tables

- Start recording when the team discussion begins. Keep cameras on and make shared prompts, responses and evidence readable. Every buddy should explain and challenge technical points.

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
| --- | --- | --- | --- | --- | --- |
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
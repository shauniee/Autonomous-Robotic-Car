# Information about tribunal 1

Architecture Defence & Integration Readiness | AI ideation, live update, AI-vetted gap check
The core objective of this tribunal is to assess the engineering reasoning and see how the team responds to a sudden change in embedded system requirements.

# Breakdown
- Pre-tribunal AI ideation. Each buddy uses AI to ideate their subsystem position before Zoom.
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

## Pre-tribunal AI Ideation

Instructions
• Each buddy must use AI before Zoom session to prepare subsystem-specific arguments, risks, and negotiation points.
• The AI output must be critiqued. Identify what is useful, unrealistic, incomplete, or requires validation.
• For Tribunal 1, compare against the Week 6 architecture freeze. For Tribunal 2, compare against actual integration evidence.

| Buddy | AI prompt used | Useful AI ideas | Weak / unrealistic AI ideas | Position before Zoom |
| --- | --- | --- | --- | --- |
| Shaun | Act as an embedded systems reviewer for my WiFi/MQTT subsystem. Compare our Week 6 design review with the latest code in code/. Challenge network ownership, telemetry freshness, reconnect behavior and stopped barcode configuration. Identify useful design decisions, unrealistic assumptions, integration risks and evidence still needed. Help me prepare arguments and negotiation points for Tribunal 1 Version A. Distinguish implemented code from tested behavior. | Separate network processing from local control; publish timestamped snapshots with validity; acknowledge configuration changes. | Treating broker connectivity or dashboard operation as proof of robot readiness. Describing subscriptions as status-only overlooks the current stopped barcode-mapping interface. | Retain kernel-owned networking and nonblocking reporting. Permit stopped barcode configuration without movement authority. Require Pico-based reconnect and mapping-acknowledgement evidence. |
| Mahesha | Act as an embedded systems reviewer for my motors, encoders and PID subsystem. Compare our Week 6 design review with the latest code in code/. Challenge exclusive motor ownership, command leases, action completion, stationary detection, braking and reset behavior. Identify useful decisions, weak assumptions, dependencies on other subsystems and required measurements. Help me prepare my position before Zoom without treating builds as physical validation  | Single motor writer; leased commands; encoder feedback; retained completion results; separate reset and start.  | Assuming zero requested speed proves stationarity, encoder counts prove ground accuracy, or configured braking proves physical stopping performance. | Motion retains exclusive motor ownership. Require measured geometry, tuned wheel PID, action accuracy and stop latency before accepting movement performance. |
| Nicholas | Act as an embedded systems reviewer for my line/barcode subsystem and Mission/Safety coordination. Compare our Week 6 design review with the latest code in code/. Challenge IR calibration, proportional guidance, barcode capture, runtime mapping, junction detection, bounded recovery and stop/reset handling. Identify useful decisions, unrealistic assumptions, shared-resource risks and missing tests. Help me prepare evidence-based arguments and negotiation points for Tribunal 1 Version A.	Separate latest line snapshots from queued barcode events; capture bars and spaces; retain barcode-to-junction context; keep Mission and Safety ownership explicit.	Assuming two-line sensors identify every junction or original route. Treating inferred Code 39 compatibility as confirmation of the issued barcode format.	Retain calibrated proportional line guidance, bounded barcode capture and separate Mission/Safety tasks. Validate print formats, lighting, speed, junction context and recovery on hardware.
| Darrius | Act as an embedded systems reviewer for my GY-511 terrain and hump-measurement subsystem. Compare our Week 6 design review with the latest code in code/. Challenge sensor capabilities, stationary calibration, encoder-derived movement, hump-height estimation and maximum-height retention. Identify useful decisions, unrealistic assumptions, cross-subsystem dependencies and reference measurements needed. Help me prepare my position before Zoom, separating proposed accuracy from measured results.	Separate movement classification, hump detection and height validity; retain the greatest accepted height; derive turn rate from encoders.	Treating GY-511 as having a gyroscope or assuming pitch-integrated chassis rise directly proves physical hump height.	Retain task-owned calibration and reference-calibrated height estimation. Validate mounted axes, acceleration rejection and ordinary/one-wheel humps against measured references.
| Jia Jing | Act as an embedded systems reviewer for my ultrasonic, servo and obstacle-profiling subsystem. Compare our Week 6 design review with the latest code in code/. Challenge stationary scanning, servo settling, forward-range freshness, missing echoes, clearance estimates and the boundary between profiling and Mission-owned bypass. Identify useful decisions, unrealistic assumptions, integration risks and physical tests needed. Help me prepare arguments and negotiation points for Tribunal 1 Version A.	Require measured stationarity before scanning; separate forward-range and profile freshness; restore center and acquire a fresh forward reading before movement.	Treating no echo as clear space or sparse scan rays as proof of complete side/rear clearance.	Retain bounded coarse/fine scans and conservative stopping. Authorize bypass only with measured vehicle geometry, braking headroom and confirmed course bounds.


## Week 6 Architecture Defence Snapshot before Zoom

Select only 5 to 7 important Week 6 architecture claims that affect integration readiness. Do not repeat the entire Week 6 document. Each row should defend one claim using evidence, remaining weakness, and the buddy responsible.

## Shared Resource Allocation Matrix

Enter at least 3 shared resources, or more if the project has additional shared buses, timers, interrupts, actuators, sensors, queues, event groups, or shared data. A resource is “shared” if more than one task, module, interrupt, or buddy can affect it. 

## Subsystem Interface and Code Structure Map

Enter at least one interface or module per buddy. Each row should identify who provides the module, who consumes it, what inputs and outputs it exposes, and how it will be validated. 

## Week 6 Assumption Closure

Enter at least one assumption per buddy, preferably copied from Week 6. Each row should show whether the assumption is still open, validated, deferred, or converted into a risk. 
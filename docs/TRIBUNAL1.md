INF2004 Embedded Systems Programming
Week 8 Tribunal 1 Template

| Item  | Details  |     |
| ----- | -------- | --- |
Purpose  Defend Week 6 architecture, justify changes, and show readiness for partial integration.
Core workflow  Pre-submission AI ideation -> live Zoom-updated version -> final AI-vetted gap check
Assessment focus  Evidence, engineering judgement, negotiation, and ability to identify missing work
Submission format  Completed document plus Zoom recording and evidence links
Team Information
| Item      |     | Details  |
| --------- | --- | -------- |
| Group ID  |     | CSC01    |
Repository Link  https://github.com/shaun-ie/Autonomous-Robotic-Car

| Buddy    | Student ID  | Primary subsystem / persona                  |
| -------- | ----------- | -------------------------------------------- |
| Shaun    | 2500629     | Network Gatekeeper: WiFi, MQTT, telemetry    |
| Mahesha  | 2503973     | Real-Time Taskmaster: motors, encoders, PID  |
Nicholas  2501552  Track Purist: IR sensors, line following, barcode
Darrius  2500360  Physics Watchdog: IMU, terrain, hump/motion events
Jia Jing  2502825  Spatial Optimizer: ultrasonic, servo, obstacle profiling

Version A: Pre-Tribunal AI Ideation
Weak /
Buddy AI prompt used Useful AI ideas unrealistic AI Position before Zoom
ideas
Shaun
Act as an embedded systems reviewer
for my WiFi/MQTT subsystem. Retain local control that does
Compare our Week 6 design review not depend on WiFi because
with the latest code. Challenge network network delays must not
ownership, telemetry freshness, Keep networking interrupt driving or stopping.
Assuming a
reconnect behavior and stopped separate from Keep networking separate from
working
barcode configuration. Identify useful control; motor control. Mission must
dashboard
design decisions, unrealistic acknowledge accept or reject mapping
proves robot
assumptions, integration risks and applied changes before an applied
readiness.
evidence still needed. Help me prepare mappings. acknowledgement is sent.
arguments and negotiation points for Reconnect and backpressure
Tribunal 1 Version A. Distinguish behaviour still requires
implemented code from tested validation.
behavior.
Mahesha
Act as an embedded systems reviewer Retain Motion as the sole
for my motors, encoders and PID motor-output owner to prevent
subsystem. Compare our Week 6 competing writes. Retain
design review with the latest code in expiring commands so an old
One motor Assuming zero
code/. Challenge exclusive motor request cannot continue
writer; time- target speed
ownership, command leases, action indefinitely. Mission must
limited means stopped,
completion, stationary detection, preserve the original sensor
commands; or encoder
braking and reset behavior. Identify timestamp when renewing
retained counts prove
useful decisions, weak assumptions, commands, and Safety must
completion ground
dependencies on other subsystems retain stop authority.
results. accuracy.
and required measurements. Help me Command timing and
prepare my position before Zoom operating speed require
without treating builds as physical agreement based on measured
validation. latency and stopping distance.
Nicholas
Act as an embedded systems reviewer Retain separate line snapshots
for my line/barcode subsystem and and queued barcode events
Mission/Safety coordination. Compare because they have different
our Week 6 design review with the delivery requirements. Mission
Assuming two
latest code in code/. Challenge IR should own navigation
Separate line sensors identify
calibration, proportional guidance, decisions; S3 should provide
readings from every junction or
barcode capture, runtime mapping, sensor evidence without writing
queued barcode barcode
junction detection, bounded recovery motors. I propose bounded
events; retain example is
and stop/reset handling. Identify useful recovery, but the team must
junction context. officially
decisions, unrealistic assumptions, agree how to identify the
confirmed.
shared-resource risks and missing original line and when to
tests. Help me prepare evidence- abandon the search.
based arguments and negotiation
points for Tribunal 1 Version A.
Darrius
Act as an embedded systems reviewer
for my GY-511 terrain and hump-
Retain separate hump
measurement subsystem. Compare
detection and height validity
our Week 6 design review with the
because detecting a hump
latest code in code/. Challenge sensor
Separate hump does not prove an accurate
capabilities, stationary calibration, Treating GY-
detection from height estimate. Only a valid
encoder-derived movement, hump- 511 as a gyro or
height validity; completed encounter should
height estimation and maximum-height chassis rise as
retain the update the maximum. S2 must
retention. Identify useful decisions, proven hump
highest valid provide valid motion data, and
unrealistic assumptions, cross- height.
result. the team must agree the
subsystem dependencies and
operating speed and
reference measurements needed. Help
reference-hump test before
me prepare my position before Zoom,
accepting an accuracy claim.
separating proposed accuracy from
measured results.

Act as an embedded systems reviewer
for my ultrasonic, servo and obstacle- Retain stationary scanning and
profiling subsystem. Compare our  separate front/profile readings
Week 6 design review with the latest  because side scans must not
code in code/. Challenge stationary  Treating no  replace fresh forward obstacle
Confirm stopped
scanning, servo settling, forward-range  echo as clear  information. Mission should
|  Jia Jing |     |     | wheels; keep  |     |     |     |
| --------- | --- | --- | ------------- | --- | --- | --- |
freshness, missing echoes, clearance  space or sparse  own bypass decisions.
forward range
estimates and the boundary between  rays as  Clearance requires measured
separate from
profiling and Mission-owned bypass.  complete  robot geometry and uncertainty
side scans.
Identify useful decisions, unrealistic  clearance.  margins. No-echo handling
assumptions, integration risks and  needs an agreed retry and stop
physical tests needed. Help me  policy based on physical
prepare arguments and negotiation  observations.
points for Tribunal 1 Version A.
Week 6 Architecture Defence Snapshot Before Zoom
|     | What the team claimed in  |     |     |     |     | Buddy  |
| --- | ------------------------- | --- | --- | --- | --- | ------ |
Week 6 artifact  Evidence available now  Still weak or untested
|     | Week 6  |     |     |     |     | defender  |
| --- | ------- | --- | --- | --- | --- | --------- |
 / Nicholas  / Mahesha
Safety first, Motion next.  Current source uses dual-core  Loaded timing and cross-
 Shaun
Task table /  Design review architecture  micro T-Kernel: application  core handoffs; resolve
architecture  also incorrectly labelled  tasks on core 1, networking on  conflicting W6 Safety-owner
|     | single core as accepted.  |     | core 0.  |     | labels.  |     |
| --- | ------------------------- | --- | -------- | --- | -------- | --- |
Motion alone drives   Mahesha
Command, feedback, stop and
Motion state  motors; commands expire  Wheel PID, ground accuracy
acknowledgement code exists;
| chart  | and results remain until  |     |     |     | and physical braking.  |     |
| ------ | ------------------------- | --- | --- | --- | ---------------------- | --- |
inhibited logic checks ran.
acknowledged.
The S3 input task runs every
Separate line readings and  10 ms; IR ADC sampling   Nicholas
Real print decoding, junction
| Line/barcode  | barcode events; decode  |     | occurs every 20 ms. Mission  |     |     |     |
| ------------- | ----------------------- | --- | ---------------------------- | --- | --- | --- |
context and queue capacity
| state chart  | outside interrupts; sample  |     | and Motion run every 20 ms.  |     |     |     |
| ------------ | --------------------------- | --- | ---------------------------- | --- | --- | --- |
at speed.
|     | every 20 ms.  |     | End-to-end response remains  |     |     |     |
| --- | ------------- | --- | ---------------------------- | --- | --- | --- |
unmeasured.
Stopped runtime mapping was
|                 | Status-only requests;  |     |                        |     | Full transport stress and  |  Shaun |
| --------------- | ---------------------- | --- | ---------------------- | --- | -------------------------- | ------ |
| Communications  |                        |     | added. Inhibited MQTT  |     |                            |        |
|                 | network loss must not  |     |                        |     | reconnect during powered   |        |
| state chart     |                        |     | mapping changes and    |     |                            |        |
|                 | block driving.         |     |                        |     | operation.                 |        |
reboot/reload were observed.
Only valid complete hump  Calibration, height-validation   Darrius
| Terrain state  |                        |     |                                 |     | Mounted calibration and   |     |
| -------------- | ---------------------- | --- | ------------------------------- | --- | ------------------------- | --- |
|                | encounters update the  |     | and history code exists; logic  |     |                           |     |
| chart          |                        |     |                                 |     | reference-hump accuracy.  |     |
|                | maximum height.        |     | checks ran without the IMU.     |     |                           |     |
Scan guards, cancellation and
Scan while stationary; keep  Real range/profile accuracy,   Jia Jing
| Obstacle state  |                             |     | center restoration exist;     |     |                       |     |
| --------------- | --------------------------- | --- | ----------------------------- | --- | --------------------- | --- |
|                 | front and profile readings  |     |                               |     | clearance and bypass  |     |
| chart           |                             |     | synthetic checks ran without  |     |                       |     |
|                 | separate                    |     |                               |     | geometry.             |     |
sonar.
 Nicholas
|                | Reset does not start    |     | Separate start/reset and       |     |                                 |     |
| -------------- | ----------------------- | --- | ------------------------------ | --- | ------------------------------- | --- |
| Mission state  |                         |     |                                |     | Stop/reset races, finish rules  |     |
|                | movement; recovery has  |     | guarded recovery exist;        |     |                                 |     |
| chart          |                         |     |                                |     | and original-line recovery.     |     |
|                | time/travel limits.     |     | inhibited Mission checks ran.  |     |                                 |     |

Shared Resource Allocation Matrix
| Shared    | Used by  | Owner task /  |              |                  |             | Evidence  |
| --------- | -------- | ------------- | ------------ | ---------------- | ----------- | --------- |
|           |          |               | Access type  | Contention risk  | Mitigation  |           |
| resource  | task(s)  | module        |              |                  |             | needed    |

One motor
Physical stop,
|             |           | Motion owns       |                 | Conflicting or  | writer; stop  |                   |
| ----------- | --------- | ----------------- | --------------- | --------------- | ------------- | ----------------- |
| Motors and  | Mission,  |                   | Command         |                 |               | expiry and reset  |
|             |           | outputs; Mission  |                 | expired         | overrides     |                   |
| command     | Safety,   |                   | writes and      |                 |               | trials; measure   |
|             |           | commands;         |                 | movement        | movement;     |                   |
| state       | Motion    |                   | feedback reads  |                 |               | the 50 ms brake-  |
|             |           | Safety stops.     |                 | requests        | commands      |                   |
output target.
expire.
|           |            |                 |              |                   | Separate slices;  | Check motor,      |
| --------- | ---------- | --------------- | ------------ | ----------------- | ----------------- | ----------------- |
|           | | Motion,  | Motion slices   |              | Resetting PWM     |                   |                   |
| PWM       |            |                 | Owned-slice  |                   | avoid resetting   | buzzer and servo  |
|           | buzzer,    | 4/5; buzzer 3;  |              | disrupts another  |                   |                   |
| hardware  |            |                 | updates      |                   | the shared        | waveforms         |
|           | servo      | servo 6.        |              | output.           |                   |                   |
|           |            |                 |              |                   | block.            | together.         |
|           | Mission,   |                 |              |                   | Check             |                   |
|           | Line/Barco |                 |              |                   | age/validity;     | Concurrent-       |
Stale/torn
| Motion    | de,       |         | Copied    |                   | bounded locks;  | reader and lock- |
| --------- | --------- | ------- | --------- | ----------------- | --------------- | ---------------- |
|           |           | Motion  |           | readings or lost  |                 |                  |
| feedback  | Terrain,  |         | snapshot  |                   | retain results  | timing checks    |
completion
|     | Obstacle,  |     |     |     | until          | under load.  |
| --- | ---------- | --- | --- | --- | -------------- | ------------ |
|     | Reporting  |     |     |     | acknowledged.  |              |
Stopped
Busy/backpressur
validation; full
|          | MQTT,     | S1 transport;  | Copied request  |                  |               | e/reconnect tests;  |
| -------- | --------- | -------------- | --------------- | ---------------- | ------------- | ------------------- |
|          |           |                |                 | Partial updates  | 26-entry      |                     |
| Barcode  | Mission,  | Mission        | and             |                  |               | inhibited           |
|          |           |                |                 | or replacement   | replacement;  |                     |
mapping  line/barcod acceptance; S3  acknowledgeme mapping/reload
|     |     |           |     | while moving  | four  |                 |
| --- | --- | --------- | --- | ------------- | ----- | --------------- |
|     | e   | storage.  | nt  |               |       | subset already  |
acknowledgeme
observed.
nt slots.
Calculate both-
Bounded
wheel edge rate;
handlers;
measure ISR
|             |           | Shared        |               |               | deferred      |                 |
| ----------- | --------- | ------------- | ------------- | ------------- | ------------- | --------------- |
|             | Encoder,  |               |               | Missed edges  |               | time, invalid   |
| GPIO        |           | dispatcher;   |               |               | decoding; µT- |                 |
|             | barcode,  |               | Edge capture  | and delayed   |               | counts and      |
| interrupts  |           | module-owned  |               |               | Kernel-       |                 |
|             | sonar     |               |               | control       |               | overflow under  |
|             |           | pins          |               |               | supported     |                 |
combined load.
interrupt
Consider PIO if
signaling.
necessary.
Agree numeric
Measure
|          |             |                |            | Deadline          | priorities/WCET;  |                     |
| -------- | ----------- | -------------- | ---------- | ----------------- | ----------------- | ------------------- |
| CPU and  | Application |                |            |                   |                   | execution, jitter,  |
|          |             | Team and task  | Scheduled  | misses, blocking  | leave             |                     |
| task     | /network    |                |            |                   |                   | missed releases,    |
|          |             | owners         | execution  | and stack         | processing time   |                     |
| memory   | tasks       |                |            |                   |                   | stack use and       |
|          |             |                |            | exhaustion        | beyond I²C        |                     |
lock duration.
timeout.
|     |     |     |     |     | Separate        | Test the 64-edge  |
| --- | --- | --- | --- | --- | --------------- | ----------------- |
|     |     |     |     |     | snapshots/event | ring and eight-   |
Capture
Line/Barco Capture ring and  Overflow at  s; define  event FIFO;
| and event  |              | S3  |             |        |           |              |
| ---------- | ------------ | --- | ----------- | ------ | --------- | ------------ |
|            | de, Mission  |     | event FIFO  | speed  | overflow  | record peak  |
capacity
|     |     |     |     |     | response and     | occupancy and  |
| --- | --- | --- | --- | --- | ---------------- | -------------- |
|     |     |     |     |     | actual storage.  | overflow.      |
Subsystem Interface and Code Structure Map
| Module / file  | Provid | Consumer |         |          |                 | Validation  |
| -------------- | ------ | -------- | ------- | -------- | --------------- | ----------- |
|                |        | Purpose  | Inputs  | Outputs  | Interface rule  |             |
| / interface    | er     | s        |         |          |                 | rule        |
Test invalid
|     |     |            |     |            | Define topics, JSON  | requests,  |
| --- | --- | ---------- | --- | ---------- | -------------------- | ---------- |
|     |     | Telemetry  |     | Telemetry  |                      |            |
S1 reporting   Shaun Mission,  fields, accepted  reconnect and
|            |     | and            | Snapshots     | and       |                      |               |
| ---------- | --- | -------------- | ------------- | --------- | -------------------- | ------------- |
| / mapping  |     | network,       |               |           | requests and retry   | backpressure  |
|            |     | permitted      | and requests  | acknowled |                      |               |
| transport  |     | laptop         |               |           | limits; no movement  | without       |
|            |     | configuration  |               | gements   |                      |               |
|            |     |                |               |           | authority.           | delaying      |
control.
Only Motion writes
motors; stop/expiry
| Motion      |          |                      |               | Motor     |                     | Measure       |
| ----------- | -------- | -------------------- | ------------- | --------- | ------------------- | ------------- |
|             |          |                      | Leased        |           | overrides           |               |
| command /   |  Mahesha | Mission,  Control    |               | output,   |                     | speed,        |
|             |          |                      | commands,     |           | commands.           |               |
| feedback /  |          | Safety;  motors and  |               | measured  |                     | distance,     |
|             |          |                      | encoder       |           | Command             |               |
| stop /      |          | feedback  report     |               | motion,   |                     | turns,        |
|             |          |                      | counts, stop  |           | acceptance, action  |               |
| acknowledge |          | readers  motion.     |               | action    |                     | completion    |
|             |          |                      | state         |           | completion, and     |               |
| ment        |          |                      |               | result    |                     | and braking.  |
stationary indication
are separate states.

Provide
|             |            |             | IR readings,  | Line       |                   | Test contrast,  |
| ----------- | ---------- | ----------- | ------------- | ---------- | ----------------- | --------------- |
| Line        |  Nicholas  | steering    |               |            | No motor writes;  |                 |
|             |            |             | edges,        | snapshot,  |                   | prints in both  |
| snapshot /  | Mission,   | guidance    |               |            | preserve sample   |                 |
|             |            |             | encoder       | queued     |                   | directions,     |
| barcode     | reporting  | and         |               |            | age and queued    |                 |
|             |            |             | travel,       | barcode    |                   | speed and       |
| event APIs  |            | navigation  |               |            | events.           |                 |
|             |            |             | mapping       | event      |                   | duplicates.     |
events.
Calibrate
|              | Mission,          |               |           | Calibration  | Terrain owns         |               |
| ------------ | ----------------- | ------------- | --------- | ------------ | -------------------- | ------------- |
| Terrain      |                   | Classify      | GY-511    |              |                      | mounted       |
|              |  Darrius Safety,  |               |           | state,       | processing; invalid  |               |
| snapshot /   |                   | movement      | samples,  |              |                      | sensor;       |
|              | calibration       |               |           | movement,    | height cannot        |               |
| calibration  |                   | and estimate  | motion    |              |                      | compare with  |
|              | coordinator       |               |           | height,      | replace a valid      |               |
| APIs         |                   | hump height.  | context   |              |                      | known         |
|              | , reporting       |               |           | maximum      | maximum.             |               |
humps.
Echoes,
Test known
|               |            |             | servo       |             | Scan while            |               |
| ------------- | ---------- | ----------- | ----------- | ----------- | --------------------- | ------------- |
| Obstacle      |  Jia Jing  | Measure     |             | Front       |                       | objects, no   |
|               |            |             | settings,   |             | stationary; separate  |               |
| snapshot /    | Mission,   | obstacles   |             | range,      |                       | echo,         |
|               |            |             | scan        |             | front/profile age;    |               |
| scan request  | Reporting  | and         |             | scan rays,  |                       | cancellation  |
|               |            |             | request,    |             | Mission owns          |               |
| / cancel      |            | clearance.  |             | profile     |                       | and fresh     |
|               |            |             | stationary  |             | bypass.               |               |
center return.
feedback
Inject faults;
Consolidate
|     |  All owners |     |     |     |     | verify  |
| --- | ----------- | --- | --- | --- | --- | ------- |
Fault  Mission,  Consistent  Faults, stale  Status and  detector, threshold,
stopping and
reporting  Safety,  fault  data and  recovery  Stopped/Fault
that reset
contract  reporting  handling  expiry  eligibility  outcome, latching
never starts
and recovery.
movement.
Week 6 Assumption Closure
Week 6
|           | Original    | Current             |     |                 |              |     |
| --------- | ----------- | ------------------- | --- | --------------- | ------------ | --- |
| assumptio |             | Evidence collected  |     | Remaining risk  | Next action  |     |
|           | assumption  | status              |     |                 |              |     |
n ID
Inhibited
|     | WiFi/MQTT       |                                   |     |                    | Shaun: interrupt WiFi/broker and  |     |
| --- | --------------- | --------------------------------- | --- | ------------------ | --------------------------------- | --- |
|     |                 | Needs  mapping/reload and         |     | Driving reconnect  |                                   |     |
| A1  | recover within  |                                   |     |                    | measure recovery and control      |     |
|     |                 | validation  laptop broker checks  |     | time is unknown.   |                                   |     |
|     | the target.     |                                   |     |                    | timing.                           |     |
ran.
Encoder counts
Manual wheel trials  Count errors
|     | and direction    |                                    |     |                 | Mahesha: repeat known              |     |
| --- | ---------------- | ---------------------------------- | --- | --------------- | ---------------------------------- | --- |
|     |                  | Needs  gave provisional            |     | corrupt motion  |                                    |     |
| A2  | remain reliable  |                                    |     |                 | revolutions at several speeds and  |     |
|     |                  | validation  counts; one trial had  |     | and barcode     |                                    |     |
|     | across speeds    |                                    |     |                 | directions.                        |     |
invalid transitions.  widths.
and reversals.
Lighting,
Mounted IR
Synthetic  mounting and
|     | gives clear   |                              |     |            | Nicholas: record real ADC ranges  |     |
| --- | ------------- | ---------------------------- | --- | ---------- | --------------------------------- | --- |
|     |               | Needs  decoder/capture       |     | speed may  |                                   |     |
| A3  | contrast and  |                              |     |            | and barcode edges across          |     |
|     |               | validation  firmware checks  |     | cause      |                                   |     |
|     | complete      |                              |     |            | conditions.                       |     |
passed.  missed/duplicate
barcode frames.
reads.
Pitch and travel
Darrius: calibrate, filter pitch and
|     | can estimate     | Needs  Logic checks without  |     | Acceleration/slip  |                                     |     |
| --- | ---------------- | ---------------------------- | --- | ------------------ | ----------------------------------- | --- |
| A4  |                  |                              |     |                    | repeat steady-speed trials against  |     |
|     | calibrated hump  | validation  IMU.             |     | distort height.    |                                     |     |
measured humps; report error.
height.
Sonar profiles  No echo may  Jia Jing: test range, no echo and
Needs  Synthetic checks
A5  represent actual  mean open  sensor faults; agree retry/stop
validation  without sonar.
obstacles.  space or failure.  policy from observed behaviour.
|     | Motor/servo  | Power policy exists;  |     | Brownout,  |     |     |
| --- | ------------ | --------------------- | --- | ---------- | --- | --- |
All owners: verify modules and
|     | load transients  | Needs  no accepted load- |     | damage or         |                                     |     |
| --- | ---------------- | ------------------------ | --- | ----------------- | ----------------------------------- | --- |
| A6  |                  |                          |     |                   | measure logic levels, loaded rails  |     |
|     | stay within      | validation  transient    |     | corrupted sensor  |                                     |     |
and currents.
|     | electrical limits.  | measurements.  |     | readings.  |     |     |
| --- | ------------------- | -------------- | --- | ---------- | --- | --- |
All owners: confirm GP27=ADC1,
|     | Grove wiring  | Pin risks  |     | ADC, console,  |     |     |
| --- | ------------- | ---------- | --- | -------------- | --- | --- |
GP0/1 stdio settings, GP28
|     | and unused      | Needs  documented;           |     | battery-sense  |                                |     |
| --- | --------------- | ---------------------------- | --- | -------------- | ------------------------------ | --- |
| A7  |                 |                              |     |                | jumper open and GP26/28 power- |     |
|     | contacts match  | validation  physical checks  |     | and Echo       |                                |     |
off continuity; measure Echo ≤3.3
|     | the design.  | pending.  |     | conflicts.  |     |     |
| --- | ------------ | --------- | --- | ----------- | --- | --- |
V before GP2 connection.
|     | Barcode  |     |     | Issued framing,  |     |     |
| --- | -------- | --- | --- | ---------------- | --- | --- |
Four-letter rule
|     | mapping,          |                               |     | direction,      |                                  |     |
| --- | ----------------- | ----------------------------- | --- | --------------- | -------------------------------- | --- |
|     |                   | Partly  confirmed; rehearsal  |     |                 | Nicholas/team: confirm course    |     |
| A8  | orientation and   |                               |     | tolerances and  |                                  |     |
|     |                   | clarified  mapping changes    |     |                 | details and test issued prints.  |     |
|     | junction offsets  |                               |     | spacing remain  |                                  |     |
and reload observed.
|     | are confirmed.  |     |     | unresolved.  |     |     |
| --- | --------------- | --- | --- | ------------ | --- | --- |

Nicholas/Jia Jing: define search
|     | The course  |     | Ambiguous lines,  |     |
| --- | ----------- | --- | ----------------- | --- |
Recovery guards  pattern, heading/corridor, stable
|     | permits bypass  |     | blocked sides  |     |
| --- | --------------- | --- | -------------- | --- |
A9  Open  exist; no physical  distance and budgets; test both-
|     | and original-line  |     | and unsuitable  |     |
| --- | ------------------ | --- | --------------- | --- |
pass.  blocked, timeout and retry
|     | recovery.  |     | limits.  |     |
| --- | ---------- | --- | -------- | --- |
exhaustion.
Finish and hump
No confirmed finish
|     | height  |     | Premature finish  |     |
| --- | ------- | --- | ----------------- | --- |
convention or  Nicholas/Darrius: confirm rules,
| A10  | assessment  | Open  | or unsupported  |     |
| ---- | ----------- | ----- | --------------- | --- |
accepted height- references and error limits.
|     | criteria are  |     | accuracy claims  |     |
| --- | ------------- | --- | ---------------- | --- |
error limit.
specified.
S3 input task: 10 ms;
Motion timing  IR ADC sampling: 20
All owners: timestamp sample,
|     | and the 50 ms  | ms; Mission and  | End-to-end  |     |
| --- | -------------- | ---------------- | ----------- | --- |
decision, command and PWM
|      | brake-output  | Needs  Motion: 20 ms.             | response  |                                  |
| ---- | ------------- | --------------------------------- | --------- | -------------------------------- |
| A11  |               |                                   |           | under load; report brake-output  |
|      | target hold   | validation  These are configured  | remains   |                                  |
latency separately from physical
|     | under peak  | periods; end-to-end  | unmeasured.  |     |
| --- | ----------- | -------------------- | ------------ | --- |
stopping.
load.  response remains
unmeasured.
Expiry, reset
rejection and  Reset or held- Nicholas/Mahesha: test expiry,
Stop/reset code and
watchdog  Needs  button behavior  old commands, held buttons,
A12  inhibited logic checks
recovery  validation  may allow unsafe  inhibited boot and stalled
exist.
|     | prevent unsafe  |     | continuation.  | progress.  |
| --- | --------------- | --- | -------------- | ---------- |
restart.
Mahesha/Nicholas: calibrate
wheel scaling and track width,
|     | Wheel geometry  |     |     | then measure ground travel,      |
| --- | --------------- | --- | --- | -------------------------------- |
|     | and bounded     |     |     | lateral tracking and turn error  |
Geometry/count  Encoder travel
|      | slip support  | Needs            |               | against external references.     |
| ---- | ------------- | ---------------- | ------------- | -------------------------------- |
| A13  |               | scaling remains  | differs from  |                                  |
|      | repeatable    | validation       |               | Proposed targets: lateral error  |
provisional.  ground travel.
|     | motion     |     |     | ≤10 mm and final turn error ≤5°.  |
| --- | ---------- | --- | --- | --------------------------------- |
|     | accuracy.  |     |     | Targets and test conditions       |
require team agreement and
physical validation.

Version B: Live Zoom-Updated Version
Buddy Contributions During Zoom
Key challenge, concern, or
| Buddy  |     | Challenge received  |     | Decision or compromise reached  |     |
| ------ | --- | ------------------- | --- | ------------------------------- | --- |
argument raised
Risk of race conditions when multiple  How to ensure safe access to  Use mutex protected snapshot and
Shaun
tasks update shared packed data.  edit the data.  run concurrency tests.
When dealing with values which  For each type of value that
require precision, the number of bits  requires precision like  Requires testing with actual robot
Mahesha  needed must be decided on early on  timestamps and speed values,  and other subsystem’s that rely on
which may affect the accuracy of the  how many bits are needed for  the precision of the data.
motor’s movement.  safe motion controls?
How to justify the number of bits
Correct bit widths;
|     |     | allocated to each field (including  |     | Document every field valid value  |     |
| --- | --- | ----------------------------------- | --- | --------------------------------- | --- |
Independent faults need separate bits
|     |     | unknown, invalid and fault  |     | and use minimum justified widths;  |     |
| --- | --- | --------------------------- | --- | ---------------------------------- | --- |
as capturing barcode and queued
| Nicholas  |     | values)  |     | Use one encoded value for each  |     |
| --------- | --- | -------- | --- | ------------------------------- | --- |
barcode faults could happen together;
|     |     | How to represent multiple faults  |     | state field and independent bits for  |     |
| --- | --- | --------------------------------- | --- | ------------------------------------- | --- |
Type of barcode is stored as unsigned
|     |     | occurring simultaneously without  |     | simultaneous faults.  |     |
| --- | --- | --------------------------------- | --- | --------------------- | --- |
int
losing any fault information?
Compromise by keeping the live
Calculation of the hump climb from
|     |     | How to justify the 32-bit decimal  |     | numbers received in a 16-bit fixed- |     |
| --- | --- | ---------------------------------- | --- | ----------------------------------- | --- |
decimals into small whole numbers
Darrius  floats based on the minimum-bit  point integer for math accuracy but
very early may result in rounding
|     |     | representation?  |     | only packing the final result into 7  |     |
| --- | --- | ---------------- | --- | ------------------------------------- | --- |
errors to pile up over time.
bits for storage and reporting.
Scanning states are easier to  How to minimize the bits used  Ensure to pack the obstacle profiles
Jia Jing  compress than obstacle profiles which  for scanning states and obstacle  using minimum bits but also ensure
requires more considerations  profiles?  accuracy is preserved
Live Decision Change Log
Original idea / Week  Change  Reason for  Evidence link /  Owner /
Updated decision
| 6 design  |     | type  | change  | timestamp  | follow-up  |
| --------- | --- | ----- | ------- | ---------- | ---------- |
Meet the bit
Major
JSON based format  Binary format  representation  47:47  Shaun
redesign
requirement
Audit every field for each
| Uses ordinary enums,  |                                    |           | Meet the     |        |         |
| --------------------- | ---------------------------------- | --------- | ------------ | ------ | ------- |
|                       | subsystem, restructuring the data  | Major     |              |        | All     |
| booleans and floats   |                                    |           | requirement  | 49:20  |         |
|                       | type to binary value instead of    | redesign  |              |        | Owners  |
| for data values       |                                    |           | change       |        |         |
original data types like uint_64
Meet the bit
|                         | Restructure fault states to bit  | Major     |                 |        | All     |
| ----------------------- | -------------------------------- | --------- | --------------- | ------ | ------- |
| Enums for fault states  |                                  |           | representation  | 51:00  |         |
|                         | values                           | redesign  |                 |        | Owners  |
requirement
| Mission requests  | Priorities to serve which  |     |     |     |      |
| ----------------- | -------------------------- | --- | --- | --- | ---- |
|                   |                            | No  |     |     | All  |
actions from all  subsystem first does not need to  -  54:20
|                     |         | Change  |     |     | owners  |
| ------------------- | ------- | ------- | --- | --- | ------- |
| subsystems          | change  |         |     |     |         |
|                     |         | No      |     |     | All     |
| Priority hierarchy  | -       |         | -   | -   |         |
|                     |         | Change  |     |     | owners  |
Subsystems
No
| communicate through  | -   |     | -   | -   | All owner  |
| -------------------- | --- | --- | --- | --- | ---------- |
Change
RTOS interfaces
Floating point values
| and signed integer  |     |     |     |     | Nicholas,  |
| ------------------- | --- | --- | --- | --- | ---------- |
Audit all related fields for each
| values are currently  |                                    |           | Meet the bit    |        | Mahesha,  |
| --------------------- | ---------------------------------- | --------- | --------------- | ------ | --------- |
|                       | subsystem, restructuring the data  | Major     |                 |        |           |
| normalized and        |                                    |           | representation  | 58:45  | Gan Jia   |
|                       | type to binary whilst ensuring     | redesign  |                 |        |           |
| handled to retain as  |                                    |           | requirement     |        | Jing,     |
values are not truncated or lost
| much accuracy as  |     |     |     |     | Darrius  |
| ----------------- | --- | --- | --- | --- | -------- |
possible.
Resource and Assumption Review
Item challenged  Evidence presented  Decision made  Impact on other buddies
Compare actual RAM and
Existing application data
message sizes before
| Memory usage   | uses enums, floats and  |     |     | Low  |     |
| -------------- | ----------------------- | --- | --- | ---- | --- |
and after using the same
integers
build settings

Measure cpu overhead
Priority of subsystems which
| Core usage  |     |     | from packing and  |     | Low  |     |
| ----------- | --- | --- | ----------------- | --- | ---- | --- |
is retained
unpacking
Audit and testing required
Subsystem numeric values  Precision of existing  from all buddies (ensure
High, affects all subsystems
and precision  representation is valid  that bits are sufficient,
avoiding bit overflow)
Multiple distance and  Quantize and pack  Mid, affects subsystem 2, 3,
Obstacle profile size
|     | clearance   |     | measurements  |     | 5   |     |
| --- | ----------- | --- | ------------- | --- | --- | --- |
Upon each barcode scan,
the encoder is requested
Minimum number of bits
No concrete total distance of  to reset the distance to  Mid, affects subsystems 2, 3,
allocated to motor encoder’s
|     | the mission  |     | avoid overflow of values  |     | 5   |     |
| --- | ------------ | --- | ------------------------- | --- | --- | --- |
max distance
to allow robot to travel
infinitely.
Risk Assessment and Subsystem Test Plan
Integration Risk Register
Risk
Risk  Cause  Impact  Likelihood  Severity  Mitigation  Owner  Status
ID
Use mutex
|            | Concurrent  |                  |     | protected  |     |     |
| ---------- | ----------- | ---------------- | --- | ---------- | --- | --- |
| Corrupted  |             | Incorrect robot  |     |            |     |     |
|            | updates to  |                  |     | snapshot   |     |     |
R1  telemetry data  status or fault  High  High  Shaun  Open
|               | shared packed  |            |     | and do       |     |     |
| ------------- | -------------- | ---------- | --- | ------------ | --- | --- |
| (Scheduling)  |                | reporting  |     |              |     |     |
|               | fields         |            |     | concurrency  |     |     |
tests
Approve
bounds and
|     | Widths exclude  | Wrong motion  |     |     |     |     |
| --- | --------------- | ------------- | --- | --- | --- | --- |
precision  Nicholas
|     | real values,  | values,  |     |     |     |     |
| --- | ------------- | -------- | --- | --- | --- | --- |
first. Specify  /
| Range/precision  | invalid codes,  | freshness     |         |                         |          |       |
| ---------------- | --------------- | ------------- | ------- | ----------------------- | -------- | ----- |
| R2               |                 |               | Medium  | Medium  invalid/out of  | Mahesha  | Open  |
| loss             | rollover, or    | decisions or  |         |                         |          |       |
range  / Darrius
|     | required  | hump  |     |     |     |     |
| --- | --------- | ----- | --- | --- | --- | --- |
behaviors  / Jia Jing
|     | resolution  | measurements  |     |     |     |     |
| --- | ----------- | ------------- | --- | --- | --- | --- |
and test
boundaries
Switch to bit
system
When dealing
incrementally
with bits, the
to better
|     | pointers need to  | Robo Pico  |     |     |     |     |
| --- | ----------------- | ---------- | --- | --- | --- | --- |
understand
|                     | reference a     | stops its      |       |                      | All      |       |
| ------------------- | --------------- | -------------- | ----- | -------------------- | -------- | ----- |
| R3  Stack overflow  |                 |                | High  | High  the source of  |          | Open  |
|                     | correct memory  | processes mid  |       |                      | buddies  |       |
any memory
|     | address to avoid  | journey.  |     |     |     |     |
| --- | ----------------- | --------- | --- | --- | --- | --- |
error if
breaking the
occurs for
subsystems
easier
debugging.
Use a single
writer,
|     | Two subsystems  | Bits that could  |     |     |     |     |
| --- | --------------- | ---------------- | --- | --- | --- | --- |
suitable
|     | update different  | hold states or  |     |     |     |     |
| --- | ----------------- | --------------- | --- | --- | --- | --- |
atomic
|                | number of bits  | data will not  |        |                 |          |       |
| -------------- | --------------- | -------------- | ------ | --------------- | -------- | ----- |
| Team mismatch  |                 |                |        | operation or    | All      |       |
| R4             | due to          | be accurate    | High   | High            |          | Open  |
| (Interface)    |                 |                |        | short critical  | buddies  |       |
|                | unregulated     | across         |        |                 |          |       |
section
|     | read-modify- | different  |     |     |     |     |
| --- | ------------ | ---------- | --- | --- | --- | --- |
protecting
|     | write operations   | subsystems   |     |     |     |     |
| --- | ------------------ | ------------ | --- | --- | --- | --- |
the complete
update
Verify Echo
| Sensor voltage  | Ultrasonic echo  | GPIO damage  |     |     |     |     |
| --------------- | ---------------- | ------------ | --- | --- | --- | --- |
Voltage
R5  incompatibility  exceeds the limit  or sensor  Medium  High  Jia Jing  open
before
| (Hardware)   | 3.3V  | failure  |     |     |     |     |
| ------------ | ----- | -------- | --- | --- | --- | --- |
connection
Subsystem Test Plan
Subsystem  Test case  Success criteria  Evidence required  Owner  Status

|              | Binary telemetry encoding  |                         | Mutex protected   |        |       |
| ------------ | -------------------------- | ----------------------- | ----------------- | ------ | ----- |
| WiFi, MQTT,  |                            | Incorrect robot status  |                   |        |       |
|              | and concurrent snapshot    |                         | snapshot and      | Shaun  | Open  |
| telemetry    |                            | or fault reporting      |                   |        |       |
|              | access                     |                         | concurrency test  |        |       |
Test motor movements,
Motion control  stopping, change direction  Correct reading  Robo pico moves
Mahesha  Open
with encoder  and providing data for other  appears in serial log.  according to plan
subsystems
Timestamped
Scan only starts after
| Obstacle +  |                           |     | sensor, motion  |           |       |
| ----------- | ------------------------- | --- | --------------- | --------- | ----- |
|             | Test stationary scanning  |     |                 | Jia Jing  | Open  |
valid stationary
| motion  |     |     | feedback and  |     |     |
| ------- | --- | --- | ------------- | --- | --- |
feedback
deadline
Calculated height
| Terrain +  |                            | matches test hump    | Serial log showing  |          |       |
| ---------- | -------------------------- | -------------------- | ------------------- | -------- | ----- |
|            | Test hump rise estimation  |                      |                     | Darrius  | Open  |
| motion     |                            | height within a few  | the data            |          |       |
millimeters.
Test for safety and timing,
Motion +  Nicholas,
|             | exercise braking, stationary  | Scanning needs fresh  |                    |               |       |
| ----------- | ----------------------------- | --------------------- | ------------------ | ------------- | ----- |
| Tracking +  |                               |                       | Synchronized logs  | Mahesha, Jia  | Open  |
|             | confirmation, no-echo and     | stationary feedback   |                    |               |       |
Obstacle  Jing
stop cancellation
Constraint Injection / Curveball Response
| Injected    | Buddy             |                          |     |     |     |
| ----------- | ----------------- | ------------------------ | --- | --- | --- |
|             | Immediate impact  | Adaptation / compromise  |     |     |     |
| constraint  | affected          |                          |     |     |     |
Existing JSON
Encode telemetry and commands in binary. Preserve mutex protected
| All  | Shaun  telemetry changes from  |     |     |     |     |
| ---- | ------------------------------ | --- | --- | --- | --- |
snapshots and test concurrency to prevent race conditions
| application  | text to binary fields  |     |     |     |     |
| ------------ | ---------------------- | --- | --- | --- | --- |
data must
Encoder and motor outputs need to be adapted to use bit
| use the  | Mahesha  Each buddy will have to  |     |     |     |     |
| -------- | --------------------------------- | --- | --- | --- | --- |
representation.
| minimum  | first determine the  |     |     |     |     |
| -------- | -------------------- | --- | --- | --- | --- |
IR states and IR data will be adjusted to binary representation to
| required  | Nicholas  different fields that  |     |     |     |     |
| --------- | -------------------------------- | --- | --- | --- | --- |
minimize bit usage
| number of  | need to be changed.  |     |     |     |     |
| ---------- | -------------------- | --- | --- | --- | --- |
Darrius  Pack terrain states, pitch and hump height into their respective bits
| bits   | Then,     |                                              |     |     |     |
| ------ | --------- | -------------------------------------------- | --- | --- | --- |
|        | Jia Jing  | Use 3-bit states and use quantized profiles  |     |     |     |
Integration Readiness Self-Assessment
| Readiness area  | Status  Evidence  |     |     | Concern  |     |
| --------------- | ----------------- | --- | --- | -------- | --- |
Stated in design review, no hardware amendment
Hardware wiring known  Green  is required given the change in requirement not  -
affect it
RTOS task ownership  Stated in design review, there is no change in task
-
Green
clear  ownership given the change in requirement
Shared resources  Identified the shared values required by all
Amber
identified  subsystems in codebase
Tested on the new change in
Critical state charts  Identified the key states required in each
|     | Amber  |     |     | requirement has not been  |     |
| --- | ------ | --- | --- | ------------------------- | --- |
implementable  subsystem.
implemented. But implementation
Safety or fault handling  Identified each subsystem’s fault states that are
|     | Amber  |     |     | of shared resources has been  |     |
| --- | ------ | --- | --- | ----------------------------- | --- |
defined  required.
identified and done up
Week 10 partial
Amber  Existing architecture and preliminary code checks
integration feasible

Updated readiness position before the Week 10 partial integration demo:
The robo pico is partially ready and has been fully integrated but no regression testing done across a few of the
subsystems.  Further testing is required to verify the functionality of all the subsystem. For Minimum-bit changes,
agree field ranges, packed layouts and safe access functions. Required RAM and message-size comparisons and
correctness/performance evidence is planned to be implemented before week 13 submission.

Version C: Final AI-Vetted Gap Check
AI reviewer prompt used
Review only Versions A and B of this tribunal document. Compare the
Week 6 architecture and open assumptions with the live Zoom
decisions, especially the minimum-bit requirement. Challenge field
ranges, precision, invalid values, simultaneous faults, shared-data
ownership, binary interfaces, encoder resets, safety timing and physical
readiness. Identify contradictions and missing evidence; retain justified
Review scope and prompt
design decisions and name the relevant owners. Distinguish recorded
decisions, existing preliminary checks and proposed follow-up. Do not
invent code changes, measurements, approvals or test results. Produce
eight detailed gap-check entries and a final tribunal summary. This
review uses the document only; the repository, recording and robot have
not been independently rechecked.

AI-identified gap  Team response  Action taken  Evidence / reason
Recorded decision:
| Retain the minimum-bit  | field audit and  |     |
| ----------------------- | ---------------- | --- |
Version B: field audit at
| objective, but interpret it  | independent fault  |     |
| ---------------------------- | ------------------ | --- |
1. Minimum-bit representation is  49:20; fault changes at
| as the smallest justified  | bits. Follow-up: each  |     |
| -------------------------- | ---------------------- | --- |
not yet a complete data contract.  51:00; precision review
| representation that  | owner documents  |     |
| -------------------- | ---------------- | --- |
Version B proposes smaller  at 58:45. Nicholas
| preserves required  | bounds, units,  |     |
| ------------------- | --------------- | --- |
fields, 3-bit scan states, 16-bit  explicitly identified
| behavior. Binary encoding  | signedness, scale,  |     |
| -------------------------- | ------------------- | --- |
terrain arithmetic and a 7-bit  simultaneous faults. No
| alone does not establish  | rounding, invalid  |     |
| ------------------------- | ------------------ | --- |
height result without agreed  completed field
| minimum storage. A  | codes and overflow  |     |
| ------------------- | ------------------- | --- |
ranges, units, resolution or  specification or passing
| single state value is  | behavior. Review  |     |
| ---------------------- | ----------------- | --- |
invalid encodings. Timestamps,  boundary results are
| suitable for mutually  | packed layouts  |     |
| ---------------------- | --------------- | --- |
signed speed, counters and  supplied. Terrain and
| exclusive states; faults  | together before  |     |
| ------------------------- | ---------------- | --- |
simultaneous faults need  obstacle widths remain
| that can coexist must  | conversion; approve  |     |
| ---------------------- | -------------------- | --- |
separate treatment.  proposals requiring
| remain independently  | widths only after  |     |
| --------------------- | ------------------ | --- |
justification.
| representable.  | boundary and  |     |
| --------------- | ------------- | --- |
precision checks.
Recorded decision:
| Retain Motion as the only  | mutex-protected      | Version A describes   |
| -------------------------- | -------------------- | --------------------- |
| motor writer, Mission as   | snapshots and        | dual-core operation,  |
| decision owner and         | concurrency checks.  | bounded locks and     |
2. Packed updates can corrupt
| Safety stop authority.  | Follow-up: Shaun  | snapshot freshness.  |
| ----------------------- | ----------------- | -------------------- |
unrelated fields or publish
| Preserve copied  | and all providers  | Version B R1 and R4  |
| ---------------- | ------------------ | -------------------- |
inconsistent snapshots. Version
| snapshots and existing  | identify every  | remain open, with  |
| ----------------------- | --------------- | ------------------ |
B retains task priorities and
ownership boundaries.  shared writer and  corruption and interface
RTOS interfaces, but this does
| Protect the complete  | storage unit; use  | mismatch rated High.  |
| --------------------- | ------------------ | --------------------- |
not prove synchronization
| shared update; do not  | one writer or an  | Required evidence  |
| ---------------------- | ----------------- | ------------------ |
remains safe when multiple
| assume separate bit  | appropriate  | includes simultaneous  |
| -------------------- | ------------ | ---------------------- |
values share storage across
| fields can be modified  | synchronized  | updates, coherent  |
| ----------------------- | ------------- | ------------------ |
application and networking
| independently or that a  | update. Measure  | reader snapshots,  |
| ------------------------ | ---------------- | ------------------ |
cores.
| mutex automatically  | lock duration,       | retained fault bits and  |
| -------------------- | -------------------- | ------------------------ |
| satisfies real-time  | blocking and cross-  | unchanged control        |
| deadlines.           | core handoffs under  | deadlines.               |
combined load.
| Treat Version B's reset  | Recorded: barcode- | Version B's defence  |
| ------------------------ | ------------------ | -------------------- |
3. Resetting encoder distance at
| idea as a segment- | triggered reset was  | table cites unknown  |
| ------------------ | -------------------- | -------------------- |
every barcode does not establish
| distance proposal  | proposed. Follow- | total mission distance  |
| ------------------ | ----------------- | ----------------------- |
overflow-safe travel. Missing a
requiring an interface  up: Mahesha  and proposes a reset at
barcode can leave an
| review. Preserve each  | coordinates with  | each barcode. Version  |
| ---------------------- | ----------------- | ---------------------- |
unbounded interval; resetting a
| consumer's continuity and  | Nicholas, Darrius  | A identifies encoder  |
| -------------------------- | ------------------ | --------------------- |
shared count can also interrupt
sign/direction meaning. A  and Jia Jing on per- travel as input to several
motion targets, barcode-width
| physical encoder reset, a  | consumer baselines  | subsystems, and  |
| -------------------------- | ------------------- | ---------------- |
measurement, terrain integration
| software counter reset  | or another bounded  | A2/A13 remain  |
| ----------------------- | ------------------- | -------------- |

and obstacle/bypass distance  and a new segment origin  counter contract.  unvalidated. No
tracking.  are different operations  Define reset/epoch  evidence demonstrates
|     | and must not be used  | notification and    | safe resets, bounded  |
| --- | --------------------- | ------------------- | --------------------- |
|     | interchangeably.      | rollover handling;  | barcode spacing or    |
|     |                       | test                | indefinite travel.    |
missed/duplicate
barcodes and resets
during active
actions.
Follow-up: Nicholas
|     | Retain original sensor  | and Mahesha verify  |     |
| --- | ----------------------- | ------------------- | --- |
|     | timestamps when         | expiry and          |     |
Version A: S3 task 10
|                                   | renewing commands,         | timestamp rollover,    |                            |
| --------------------------------- | -------------------------- | ---------------------- | -------------------------- |
| 4. Representation changes may     |                            |                        | ms; IR sampling,           |
|                                   | expiring leases, stop      | stale/old commands,    |                            |
| invalidate command freshness      |                            |                        | Mission and Motion 20      |
|                                   | precedence, retained       | stop/reset races,      |                            |
| and stopping guarantees.          |                            |                        | ms; end-to-end             |
|                                   | action results and         | held buttons and       |                            |
| Shortened timestamps,             |                            |                        | response unmeasured.       |
|                                   | separate reset/start       | inhibited reboot. All  |                            |
| quantized speed and packing       |                            |                        | The 50 ms brake-output     |
|                                   | behavior. Command          | owners timestamp       |                            |
| overhead can alter expiry checks  |                            |                        | figure is a target, not a  |
|                                   | acceptance, completion     | sample, decision,      |                            |
| or delay Safety/Motion even       |                            |                        | measured physical stop.    |
|                                   | and measured stationary    | command and PWM        |                            |
| when the configured task periods  |                            |                        | A11/A12 and Version        |
|                                   | feedback remain distinct.  | under peak load,       |                            |
| and priorities are unchanged.     |                            |                        | B's integrated safety      |
|                                   | A zero target is           | then measure           |                            |
tests remain open.
|     | insufficient evidence that  | stopping          |     |
| --- | --------------------------- | ----------------- | --- |
|     | wheels have stopped.        | time/distance at  |     |
agreed speeds.
Recorded: binary
format selected at
|     | Retain local control     | 47:47. Follow-up:     | Version A specifies full  |
| --- | ------------------------ | --------------------- | ------------------------- |
|     | independent of WiFi and  | Shaun defines the     | 26-entry replacement      |
|     | Mission-owned            | wire schema with all  | and four                  |
5. JSON-to-binary conversion
acceptance of stopped  owners and updates  acknowledgement slots;
needs an explicit compatible
|     | mapping changes.  | both endpoints.  | inhibited mapping  |
| --- | ----------------- | ---------------- | ------------------ |
protocol. A binary payload
|     | Networking has no  | Validate length,  | changes and  |
| --- | ------------------ | ----------------- | ------------ |
without field order, lengths, bit
|     | movement authority.  | range and version;  | reboot/reload were  |
| --- | -------------------- | ------------------- | ------------------- |
order, scaling, validity and
|     | Applied  | test  | observed. Its interface  |
| --- | -------- | ----- | ------------------------ |
version rules can be decoded
acknowledgements must  malformed/truncated  map still refers to JSON.
differently by firmware and the
|     | mean the complete  | input, busy/retry,  | These earlier results do  |
| --- | ------------------ | ------------------- | ------------------------- |
laptop, or accepted as a
|     | mapping was accepted  | reconnect and full  | not validate the new  |
| --- | --------------------- | ------------------- | --------------------- |
malformed configuration request.
|     | and installed; receipt or  | acknowledgement  | binary transport or  |
| --- | -------------------------- | ---------------- | -------------------- |
|     | successful decoding        | slots. Preserve  | powered-operation    |
|     | alone is insufficient.     | stopped-only     | reconnect behavior.  |
replacement
semantics.
Follow-up: Nicholas
and Jia Jing test
Retain separate line
|     |     | issued prints,  | Version A cites a 64- |
| --- | --- | --------------- | --------------------- |
snapshots/barcode
|     |     | directions and  | edge ring and eight- |
| --- | --- | --------------- | -------------------- |
events and separate
6. Physical line/obstacle behavior  speeds; record  event FIFO, synthetic
front/profile ages. Keep
| remains a gate despite packed  |     | queue  | decoder/sonar checks,  |
| ------------------------------ | --- | ------ | ---------------------- |
navigation and bypass
states. Compressed data does  occupancy/overflow  open course/finish rules
decisions in Mission.
not resolve IR contrast, real  and scan  and pending wiring/load
Unknown/no echo must
barcode framing, junction  cancellation/center  checks. Version B R5
remain distinct from clear
| ambiguity, no echo, servo  |     | return. With  | remains open (Echo  |
| -------------------------- | --- | ------------- | ------------------- |
space. Recovery must
settling, actual clearance or the  Mahesha, verify  above 3.3 V), so its
have agreed
need for fresh stationary and  stationary gating  Green wiring label does
heading/corridor checks,
| forward-range feedback.  |     | and ground  | not close the  |
| ------------------------ | --- | ----------- | -------------- |
time/travel limits and a
|     |     | geometry. Before  | documented electrical  |
| --- | --- | ----------------- | ---------------------- |
safe outcome when
|     |     | powered trials,  | checks.  |
| --- | --- | ---------------- | -------- |
blocked or exhausted.
confirm Echo level,
pin/jumper wiring

and loaded supply
behavior.
Recorded: Darrius
Retain separate hump proposed 16-bit
Version A reports terrain
detection and height working values and
logic checks without the
validity, and update the a 7-bit result.
IMU and leaves A4/A10
7. Terrain precision and height maximum only after a Follow-up: define
open. Version B's 'few
validity are still unproven. The valid completed units/range and
millimeters' criterion is
16-bit fixed-point/7-bit final-result encounter. Treat the error budget; check
not a confirmed
compromise lacks a stated scale, proposed widths as accumulation and
assessment tolerance or
signed range, intermediate provisional. Preserve overflow. Calibrate
measured result.
arithmetic capacity, rounding sufficient intermediate the mounted sensor,
Record actual reference
policy and representation for an precision; packing a final confirm motion-data
height, speed, repeats
invalid or out-of-range height. value must not silently continuity and
and error before
truncate, wrap or replace compare repeated
accepting accuracy or
an existing valid steady-speed runs
the packed width.
maximum. with measured
reference humps.
Follow-up: all
Record a conditional
owners link each Version B explicitly
partial-integration
change to a test, leaves risks and tests
position: documented
measurable criterion Open; the telemetry
8. Readiness and resource architecture and
and result before 'success criterion'
claims need a traceable preliminary checks
relying on it in the describes a failure and
acceptance baseline. Version B support controlled
Week 10 demo. needs a positive pass
says the robot is fully integrated integration, while
Shaun compares condition. Version A
but partially ready, while changed representations
RAM/message sizes records pending
regression and minimum-bit and physical behavior
using identical physical, timing and
evidence are pending. Green remain unvalidated.
builds; owners ownership clarifications.
ownership/wiring labels and Retain unchanged
measure packing Zoom timestamps
planned Week 13 evidence do ownership/priorities as
cost and stack use. establish decisions only.
not demonstrate Week 10 design decisions, with
Separate R3 stack This review records
readiness. loaded timing still to be
exhaustion from required work; no new
shown. Do not claim RAM
pointer/bounds implementation or tests
savings or complete
errors and track were performed.
regression.
both appropriately.
Final Tribunal Summary
Final question Team answer
The strongest part is the separation of authority and data delivery.
Motion alone writes motor outputs; Mission owns navigation and
accepted configuration changes; Safety retains stop authority;
networking does not control movement. Copied sensor snapshots
are separate from queued barcode events and retained action
results. Front obstacle data is kept separate from side-scan
What is the strongest part of the current
profiles, while hump detection is separated from height validity.
design?
These decisions give the minimum-bit redesign a sound
boundary: representation may change, but ownership, freshness,
validity and acknowledgement semantics must be preserved. This
is an architectural strength supported by the descriptions and
preliminary checks in Version A, not proof of complete physical
performance.
The highest immediate integration risk is incorrect packed-data
behavior: undersized ranges, precision loss, rollover or
simultaneous writes can produce plausible but wrong status and
What is most likely to fail?
motion inputs. Version B rates shared-data corruption and
interface mismatch High. Barcode-triggered distance resets are a
particular cross-subsystem risk because consumers may lose a

continuous reference, and a missed barcode defeats the
proposed reset interval. During powered operation, stale control
data, braking delay, unreliable no-echo interpretation or invalid
clearance can turn these errors into unsafe behavior. Encoder
accuracy, real barcode capture and hump accuracy are also still
open. Their actual failure likelihood cannot be ranked without
regression and physical measurements.
Version A records existing command/feedback/stop logic,
inhibited Motion and Mission checks, synthetic line/barcode and
obstacle checks, terrain logic checks without the IMU, and
inhibited MQTT mapping/reload observations. Version B records
agreed redesign directions and identifies owners and risks.
Together these support a controlled partial-integration attempt,
subject to the outstanding electrical and safety checks. They do
What evidence supports readiness?
not establish completion of the packed representation, measured
RAM savings, loaded timing, physical stopping or full robot
regression. Readiness should remain conditional until each
changed field/interface has a specification, a positive pass
criterion and linked results, followed by combined-load and
physical trials. Use Zoom timestamps as decision references;
attach actual build/log/video evidence separately.
The pre-Zoom AI positions focused on subsystem separation,
freshness, stopping and physical validation. The human
discussion introduced the minimum-bit constraint and its system-
wide consequences: replacing JSON with binary payloads;
justifying each field width; preserving independent simultaneous
faults; balancing terrain arithmetic precision against compact
What did AI miss that human discussion reporting; quantizing obstacle profiles; and addressing encoder
revealed? distance when total mission length is unknown. It also retained
task priorities and RTOS interfaces rather than redesigning
authority. Version C identifies the missing contracts and tests
behind those choices; it does not treat the proposed 16-bit/7-bit
values or barcode resets as proven solutions. The discussion
established requirements and compromises that the earlier AI
preparation had not resolved.
The post-discussion review supports the earlier warnings that an
implemented interface, successful build, synthetic test or
functioning dashboard cannot establish powered robot readiness.
It also identifies concrete gaps introduced by Version B: absent
field ranges/scales and invalid encodings, shared packed-update
races, unsafe reset assumptions, undefined binary compatibility,
precision/overflow risks, and missing before/after resource
What did AI correctly identify after discussion? measurements. Unchanged priorities do not establish unchanged
timing. The recorded Green wiring status needs reconciliation
with pending Echo, jumper and supply checks; the 50 ms brake-
output target and 'few millimeters' height criterion require
measurements and agreed conditions. These are findings from
comparing A and B, not independently verified code defects or
new test results. Owners must close them with traceable
evidence.
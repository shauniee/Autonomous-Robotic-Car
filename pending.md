# Pending work

## Contents

- [How this list is ordered](#how-this-list-is-ordered)
- [Where things stand](#where-things-stand)
- [Resolution plan](#resolution-plan)
- [Required coverage](#required-coverage)
- [What cannot be resolved without the robot](#what-cannot-be-resolved-without-the-robot)
- [Minimum-bit priorities](#minimum-bit-priorities)
- [1. Safety and electrical](#1-safety-and-electrical)
- [2. Motion calibration](#2-motion-calibration)
- [3. Line tracking (M1)](#3-line-tracking-m1)
- [4. Barcode and junctions (M2, M3)](#4-barcode-and-junctions-m2-m3)
- [5. Obstacle and bypass (M6 to M9)](#5-obstacle-and-bypass-m6-to-m9)
- [6. Hump (M4, M5)](#6-hump-m4-m5)
- [7. Telemetry and mapping (M10)](#7-telemetry-and-mapping-m10)
- [8. Faults seen and not explained](#8-faults-seen-and-not-explained)
- [9. Full mission and repeatability](#9-full-mission-and-repeatability)
- [10. Week 13 evidence (minimum-bit, timing, resources)](#10-week-13-evidence-minimum-bit-timing-resources)
- [11. Reports, demonstrations and traceability](#11-reports-demonstrations-and-traceability)
- [12. Stretch goal](#12-stretch-goal)
- [13. Housekeeping](#13-housekeeping)
- [Complete](#complete)
- [Superseded](#superseded)

## How this list is ordered

Most important first, in sections and within each section. The order is by importance, not the quickest next step:

1. What can damage the robot or make it unsafe.
2. What every other test depends on.
3. Course features, because the demonstration scores features working. Obstacle comes before hump because the professor's clarification calls bypass and recovery central, and bypass has never run.
4. Whole-mission repeatability, then required Week 13 evidence.
5. Reports, stretch goals and housekeeping.

IDs: `M` requirement, `MR` readiness check, `D` pending decision, `T` integrated trial, `A` Tribunal assumption, `R` Tribunal risk, `Gap` Tribunal Version C. `docs/tribunal1.md` is frozen; differences from it are listed under [Superseded](#superseded).

This is the single live completion plan. Issue #117 stays closed; no replacement
issue is needed. Work is on `fix/pending-nic2`, based on `7275347`; PR 149 is
already merged as `99fe0d8`. No edits, commits or merges go to `main` in this task.
All questions and decisions go to Nicholas; subsystem names below identify code,
not another person's approval. Confirmed acceptance/profile decisions are owned
by [project-specs](docs/project-specs.md#demo-and-full-course-decisions-2026-11-03).

## Where things stand

| Area | State | Evidence |
| --- | --- | --- |
| Firmware build | Branch baseline passes host suite, all four WATCHDOG/TRACE TEST combinations and normal course build. Not flashed. | `3c10232`, INT-162–167 |
| Safety and electrical | No recorded result. Wiring is user-confirmed only. | MR-01 to MR-05 |
| Watchdog | Code merged, off by default, never run on a Pico. No reset policy agreed. | `99fe0d8`, D15 |
| Line tracking | Good laps seen, but the last run went off course and was not smooth. No measured success rate. | User observation, INT-136, INT-139b, INT-158; cause not analysed |
| Barcode | User reports fully functional. All letters and a flipped print not tested. | User observation, INT-156 |
| Junctions | Some turns passed; junction failures also logged. | INT-137, INT-139b, INT-153 |
| Obstacle | Scans ran on real boxes with a measured width error. Bench check pending, and bypass has never run. | S5-U10, S5-U11 |
| Hump | Latest fix built, never run on the car. Last run counted 5 for 1 crossing. | INT-158, host and build only |
| Telemetry | Binary link in use on the bench. Physical subscription check pending. | S1-M01 |
| Full mission | Never run as an acceptance check. | MR-35 to MR-40 |
| Timing and resources | Not measured on the final build. | SYS-11, SYS-12 (October builds) |

## Resolution plan

This revision plans the remaining implementation and validation; it does not
claim new builds, tests or robot passes. Execute in order, completing each
software correction with a focused regression before physical trials. The
2026-11-04 demo is a milestone, not a waiver of required features or evidence.

| Step | Work and affected files | Completion evidence / dependency |
| --- | --- | --- |
| P0 — baseline and decisions | Use this file throughout; keep Tribunal frozen. Nicholas settles D01/D02 print/pause details, D05/D10 accuracy limits, D09 buffer wording/network settings, D12 course applicability, 100 mm/s scope during slower bypass, and D15 watchdog policy. Record answers only in project-specs. | Branch created; active links updated. Unknown values remain pending, not guessed. No need to contact other owners. |
| P1 — software baseline | Build the candidate with all four WATCHDOG/TRACE combinations using the documented wireless profile, including a TEST image. Run `testing/pure/run.sh`; record exact source/profile and final image checksum under testing without saving build/host output. Fix compilation/regression failures before tuning. | Arm build plus host result rows; neither is a Pico pass. Confirm stall guard off for demo and on for the separately identified full-course image. Audit build flag support before claiming a new profile switch exists. |
| P2 — existing failures | Analyse INT-157's off-course tail using acquisition-time WHEEL/line records, not bursty laptop arrival rates; compare INT-156 SCAN→STOPPED with the precise guard/release condition. Trace INT-154 calibration failure, barcode overflow and missing event through their producers/consumers. Inspect INT-122/133 stuck hump windows. Leave the ten battery-related restarts out. | A supported cause or explicitly unresolved hypothesis per symptom, linked to source/capture. Add smallest reproducing fixtures under testing; fix the shared producer/consumer only after reproduction. No invented hardware diagnosis. |
| P3 — tracking, barcode and navigation | Review `line.c`, `barcode.c`, `mission_logic.c`, `motion_control.c`, `report.c` and related host tests: curve/junction qualification, onset skew, pivot hand-over, queued-action lifetime, C/P bidirectional decoding, truncated guards, overflow retention and exactly one event per read. Audit PR 149 fallback against tests before adding coverage. Update scorer input/procedure to enforce the agreed lap conditions. | Host regression for each demonstrated defect; scorer must not infer physical success from telemetry alone. Separate lighting datasets and image profiles; unique run IDs. Robot gate: sections 2–4 and 8. |
| P4 — terrain and obstacle completeness | Review `terrain_algo.c`/`terrain.c` for completed-encounter counting, turn rejection, stuck windows, invalid height/max retention and boot calibration. Review `obstacle.c`, `obstacle_profile.c`, `bypass_control.c` and Mission for fresh stationary gating, no-echo policy, cancellation/centering, blocked sides, rear-edge depth and original-line rejoin. | Host fixtures for relevant failure paths; no threshold/geometry release without physical evidence. Verify whether the existing fix removes 5-for-1 hump counting on the current image before adding another fix. Bypass remains a required feature, not a stretch goal. |
| P5 — compact data and communication | Apply the ranked [minimum-bit priorities](#minimum-bit-priorities). Census all persistent storage, including private state, diagnostics, queues and watchdog state. Fix stale rows, use target size probes, preserve ownership/invalid codes and update both protocol endpoints if the wire changes. Audit frozen producers, expiry, full buffers, mapping BUSY/reboot rules and cross-core handoffs. | Boundary/rollover/invalid/concurrency host checks, final Arm layout and static RAM; physical timing/stack/link acceptance remains P7/P8. Resolve event-loss observability before accepting route reconstruction. |
| P6 — safe physical commissioning | With the robot: electrical checks, TEST inhibition, reset/start/stop/impact, motor direction, encoders, wheel/track geometry, loaded PID, IR polarity/lighting, sonar/servo and mounted IMU. Nicholas records all calibration and chosen limits. | Sections 1–2; MR-01–25 as applicable. Watchdog is opt-in bench work until D15 passes. Demo stall exception never disables other guards. |
| P7 — feature and lap trials | Run all section 3–7 cases: A–Z in both directions, all four actions and no-command crossing, reference humps, obstacle ranges/width/depth, safe bypass/rejoin, actual MQTT and faults. Then run the agreed lap; score 20 unchanged-image attempts per lighting condition for the existing 95% sample target (at least 19/20), and compare equal TRACE-on/off batches. | Captures plus video/reference measurements, including failures. Three unchanged-image full-course successes remain separate MR-36/37 evidence. Line-loss/blocked-route/GP21 safety cases are separate intentional-stop trials, not qualifying laps. |
| P8 — submission and closeout | Measure final combined-load WCET/jitter/deadlines, sensor-to-output and brake latency, IRQ/lock duration, packing CPU cost, stack/peak RAM and buffer occupancy. Record exact final RAM/message sizes; rerun changed functionality. Complete route reconstruction, motion/PID and terrain analyses in owning subsystem READMEs, integrated/telemetry demo evidence and video index. Recover Week 10 evidence only if it exists. | Sections 9–11, T14–T16, MR-35–40. No fabricated retrospective pass; missing historical evidence stays labelled missing. Update readiness from actual results. |
| P9 — optional closeout | Keep magnetometer fusion optional. Reconcile diagnostic-frame wording with Nicholas. Keep this completion record; leave old merged branch deletion outside the task. | Sections 12–13 have an explicit disposition; optional work cannot delay required completion. |

## Required coverage

`docs/tribunal1.md` has no table labelled FR/NFR. Until Nicholas identifies another
source, this plan conservatively retains all M1–M10 from the owning requirements
guide plus the Tribunal's functional, safety and resource obligations. These
labels are a crosswalk, not invented instructor requirement IDs.

| Requirement / obligation | Plan coverage and closure |
| --- | --- |
| M1 line tracking and lighting tolerance | P2/P3/P6/P7; sections 2–3; qualifying lap plus measured repeatability |
| M2 A–Z in either direction, stored until next junction | P3/P7; section 4; both traversal directions, one action/event per read, physical print coverage |
| M3 all four encoder actions, including one wheel on hump | P3/P6/P7; sections 2, 4 and 6; ground distance/angle measurements and navigation trials |
| M4/M5 detection, each height/count and retained maximum | P4/P6/P7; section 6; reference measurements, exact crossing count, no pivot false positives |
| M6/M7 obstacle detection, profile, clearance and depth | P4/P6/P7; section 5; measured boards/boxes and side/rear-edge evidence |
| M8/M9 safe bypass and original-line continuation | P4/P7; section 5; both sides, restricted/blocked cases, envelope and correct rejoin. Rejoining during BYPASS/NAVIGATE is required; a standalone REACQUIRE during the qualifying lap fails it. |
| M10 two-way MQTT, runtime mapping, readable telemetry and route evidence | P5/P7/P8; section 7; instructor broker, malformed requests, BUSY/reboot, fault visibility and reconnect |
| Autonomous, undisclosed route, endless operation, micro T-Kernel | P7/P8; sections 9/11; unfamiliar runtime letters, repeated encounters, no remote driving or hard-coded route |
| Single motor writer, stop precedence, freshness, reset and fault independence | P2/P5/P6/P8; sections 1/8/10; separate intentional fault/stop tests; networking has no movement authority |
| Race-free minimum-bit data, precision, invalid/overflow behavior | P5/P8; section 10 and Tribunal Gaps 1–7; field census, accessor/boundary checks, final sizes and on-target timing |
| Electrical, timing, stack, performance and repeatability | P6/P7/P8; sections 1/9/10 and Tribunal risks R1–R5; measured values, not host inference |
| Reports, demonstrations and traceability | P8; section 11 and Tribunal Gap 8; unique result IDs, exact source/image, saved measurements/video links |

## What cannot be resolved without the robot

Software investigation, fixes, host tests, Arm builds, static RAM/layout analysis,
protocol fixtures, report structure and procedure preparation can happen now.
They cannot close these physical items:

| Blocked acceptance | Why the robot or actual environment is needed | Checklist coverage |
| --- | --- | --- |
| Electrical and stopping safety | Continuity, Echo voltage, power rails under load, braking distance/output waveforms, impact, held-button/reset and watchdog behavior need hardware measurements. The loose-battery restart cause is already explained. | Section 1; MR-01–07, MR-16, MR-24/25, SYS-06 |
| Motion and encoder calibration | Actual wheel travel/sign, edge errors/rates, geometry, floor PID, slip and turn accuracy cannot be inferred from raised-wheel or host tests. | Section 2; MR-14/15, MR-19–23 |
| Robust line/barcode/junction behavior | Mounted sensor levels, lighting, issued prints, physical overflow rate, three curves, action hand-over, TRACE effect and untouched 100 mm/s laps require the course. | Sections 3–4, 8; MR-12/13, MR-26–28/32 |
| Hump accuracy and reliability | Current fix needs actual crossings; height scale/error, reverse/curve/pivot effects, minimum detectable hump, stuck detection and disconnected-IMU behavior require measured references. | Section 6; MR-11/16/23/29 |
| Obstacle and full bypass | Servo endpoints, sonar error, swept envelope, width/depth, blocked sides and original-line rejoin have no accepted integrated physical evidence. | Section 5; MR-17/18/30/31 |
| End-to-end instructor telemetry | Laptop fixtures can prove codec behavior, not Pico radio timing, reconnect, real mapping/fault injection or access to the instructor network. Credentials alone do not prove connectivity. | Section 7; MR-08–10/33/38 |
| Loaded resources and repeatability | WCET/jitter/ISR/lock cost, live stack/peak RAM, actual queue high-water marks, three full runs, 20-lap samples and physical videos require the running final image. | Sections 9–11; MR-35–40 |

Separately blocked on Nicholas, not on hardware access: unconfirmed numeric
accuracy/recovery limits, print polarity/scope, course assumptions, watchdog
policy and buffer wording. Instructor settings can be entered when available;
no application-code redesign is planned for that configuration change.

## Minimum-bit priorities

The source-based calculations, proposed layouts and limits are owned by the
[data audit](docs/project-specs.md#minimum-bit-audit-2026-11-03); no RAM savings
have been implemented or measured in this planning revision.

1. Correct the census first: include private state, stored floats, watchdog
   copies, diagnostic rings and alignment; fix signed-angle undercounts.
2. Preserve precision while packing the barcode edge ring and private flags.
   Retain ring capacity; frequent overflow is a reason to investigate, not shrink it.
3. Pack 12 independent wire fault bits into two bytes only with a coordinated
   schema update, laptop decoder/dashboard changes and golden-vector regressions.
4. Pack ray angle/validity losslessly; consider smaller sonar/profile fields only
   after proving every producer's range and invalid behavior. Quantization needs
   an agreed measurement error budget.
5. Bound stored PID values before fixed-point conversion; validate loaded control
   after conversion. Keep 32-bit freshness timestamps and cross-core publication
   words unless a safe narrower lifetime/atomic contract is proven.
6. Prove final logical bits, target storage, static RAM and live performance.
   Existing README subtotals omit private objects and cannot establish a complete
   minimum-bit implementation.

## 1. Safety and electrical

- [x] Build baseline `3c10232` with the Arm compiler: all four WATCHDOG/TRACE TEST combinations plus normal course image pass (INT-163–167). Rebuild the final candidate after source changes; no flash or physical PASS.
- [ ] Sonar Echo high level measured at 3.3 V or below (MR-03, R5, D05). A higher level can damage the GPIO.
- [ ] Every wire checked by continuity, with a photo (MR-01, A7).
- [ ] GP26 Grove contact disconnected and GP28 jumper open (MR-02, A7).
- [ ] Power bank polarity, and one power source at a time (MR-04).
- [ ] 3V3 and VSYS measured with both motors and the servo running; no brownout (MR-05, A6).
- [ ] GP21 at follow speed: robot stops, and the stopping distance is recorded (MR-24, D08).
- [ ] Measure output-brake latency against the 50 ms target, separately from physical stopping (A11, T02).
- [ ] Reset cannot start the robot: held buttons, persistent fault, expired and old commands, inhibited boot (MR-07, A12, Gap 4).
- [ ] TEST image on the current build: `[TEST]` lines print and motors stay braked (MR-06).
- [ ] Stall guard: keep it off for the 2026-11-04 demo; enable and physically verify it for full-course acceptance (MR-25, D07; confirmed profile decision in project-specs).
- [ ] Watchdog: agree the reset policy (D15), then test a `WATCHDOG=1 TEST=1` image with wheels raised: reset on a stopped task, motor outputs through reset, no restart without GP20, and no false reset under WiFi load (SYS-06). The code is merged but off by default; do not enable it on a driving car until this passes.
- [ ] Impact: tap test gives `Terrain: impact` and a latched stop (MR-16, D06).

## 2. Motion calibration

Turns, distances and barcode widths all come from encoder travel.

- [ ] Each motor drives forward on a short command (MR-19, D03).
- [ ] Encoder counts per revolution, both wheels, repeated across speeds and reversals (MR-14, A2, D03). Only manual trials so far.
- [ ] Forced encoder fault appears and clears (MR-15).
- [ ] Measure wheel diameter and effective track width; only wheel-centre separation is recorded (MR-20, D03).
- [ ] 1 m straight, five times: distance and lateral error, Tribunal proposal at most 10 mm (MR-21, A13, D07).
- [ ] 90 left, 90 right and 180 degrees, five each: heading error, Tribunal proposal at most 5 degrees (MR-22, A13, D07).
- [ ] Loaded wheel PID response on the floor; INT-100 to INT-102 used raised wheels only (D07).
- [ ] Encoder width of 16 bits confirmed with the robot, including the maximum edge rate (D03, Mahesha's Tribunal item).

## 3. Line tracking (M1)

The 95% target is the user's. No success rate has been measured yet, so the first job is to get one.

- [x] Qualifying-lap definition agreed with Nicholas; apply the [acceptance contract](docs/project-specs.md#demo-and-full-course-decisions-2026-11-03). Height accuracy still needs the D10 limit before an accuracy PASS.
- [ ] Run 20 laps with one image and no changes, record each lap, and calculate the rate with `scripts/mission_trial_score.py` (MR-26).
- [ ] Repeat under two lighting conditions (MR-26, M1).
- [ ] Analyse the off-course run in `testing/integration/logs/2026-11-02_raw-captures/int-157-run.txt.gz`, near the end of the run. `scripts/triage_mqtt_capture.py` flags anomalies but does not find causes.
- [ ] Real line loss: the sweep finds the line, an exhausted search stops, and stale data does not start a blind search (MR-32, T11).
- [ ] Work through the [open issues](app_program/line_barcode/README.md#open-issues): junction on three curves, `NAV_ONSET_SKEW_MM`, pivot hand-over.
- [ ] Mounted IR levels, dead zone and left-sensor span at course lighting (MR-12 as revised for DOUT, D04, A3).
- [ ] Check whether the diagnostic traces affect tracking: compare the same number of laps on `TRACE=1` and `TRACE=0` images. The build option exists; no laps have been run.
- [ ] Change one value per trial and record each as a new INT entry.

## 4. Barcode and junctions (M2, M3)

- [ ] Action once at the junction for each of left, right, straight and U-turn at variable spacing, never undone (MR-28, D02). INT-153 stopped with a queued turn.
- [ ] Print all 26 letters and run each at course speed, in both directions (MR-27, M2). Host tests cover the patterns only.
- [ ] Flipped print, reading it from the other direction. C and P can complete early with an unknown orientation; nothing reads orientation today.
- [ ] U-turn: confirm the stationary pause and return-line tracking. The pause is a 100 ms trial value; about 200 ms was requested (D02).
- [ ] No queued turn: crosses straight and resumes tracking (MR-28).
- [ ] Record reads against passes as a rate. A corrupted last guard still loses the read (INT-155, third pass).
- [ ] Barcode sensor level over black and white on the mounted robot (MR-13, D04).
- [ ] Issued print framing, quiet zones and tolerances (D01, A8).
- [ ] Inverted print (white bars on black), if the course can have one. Decoding now needs black high (`BARCODE_BLACK_HIGH`), so an inverted print **will not read**. Decide whether this is needed.
- [ ] Barcode capture ring and event queue: peak use and overflow at course speed (T04).

## 5. Obstacle and bypass (M6 to M9)

Wheels stay off for the bench steps. Bypass has never run on the car.

- [ ] S5-U10 with `./flash.sh test`: servo centre and limits, with +/-60 degrees unconfirmed (MR-18, D05).
- [ ] Sonar against a board at 100, 250, 500 and 1000 mm, plus open space; MR-17 is still PENDING without a saved measurement (A5, D05).
- [ ] Boxes of 100, 150 and 200 mm: check the `[SCAN]` rays and profile against a tape. S5-U11 used 150 and 220 mm, with mean width error of about 18 mm and no agreed target (M7).
- [ ] Both sides blocked: the robot stops rather than bypasses (MR-31).
- [ ] Bypass runs with both sides open and one restricted: no collision, larger-gap side chosen, swept envelope confirmed (MR-30, M8, D03).
- [ ] Rejoin the original line and continue (MR-30, M9, A9).
- [ ] Depth measured against a tape, and the rear edge seen while bypassing (M7).
- [ ] Confirm scans start only after fresh stationary feedback, with timestamped logs (Tribunal test plan, T09).
- [ ] Cancel a scan and restore the servo to centre on a stop, with no-echo cases (Tribunal test plan, Gap 6).
- [ ] Measure bypass speeds, settle time and travel budgets (D05, D07).
- [ ] Explain `SCAN` then `STOPPED` with `front=no echo` on broad black (INT-156 capture, from about line 8020).
- [ ] Confirm obstacles only sit on straights (D12, A9). The course source is inconsistent; blocked on an answer.

## 6. Hump (M4, M5)

Flash the current image, let the car sit still on the track for a second, then press GP20.

- [ ] Cross the 10 mm hump at least 3 times with barcode turns in the lap. Pass: `humps=` matches the crossings exactly, and pivots add nothing (INT-158 bug, M4).
- [ ] Reference humps from the user-selected 10 mm minimum up to the 30 mm limit, three passes each, reported against actual height (MR-29, M5, A4, D06). `HUMP_CALIB_FACTOR` 0.8 comes from one hump height.
- [ ] Agree a height error limit; "a few millimetres" is not an accepted target (A10, D10, Gap 7).
- [ ] Turn accuracy with one wheel on a hump (MR-23, M3).
- [ ] Hump on a curve, and a hump just before a barcode.
- [ ] Reverse direction over a hump: the side and roll sign are untested (D11).
- [ ] Revisit `HUMP_MIN_TRAVEL_MM` 50: it rests on only two crossings.
- [ ] Detector stuck on: a stuck encounter can still miss results; cause not investigated (INT-122, INT-133).
- [ ] Mounted calibration success and failure, with IMU disconnected (MR-11, D06).
- [ ] Labelled traces for stationary, acceleration, turn, climb, descent and impact (T06). No monitor named for turn rate was found.

## 7. Telemetry and mapping (M10)

- [ ] Configure ignored network secrets for the instructor network/broker when supplied (D09). Keep application code/schema unchanged; perform the connection/mapping/subscriber smoke check on that network.
- [ ] Pico telemetry at about 500 ms in the current binary format, saved (MR-08, S1-M01, Gap 5).
- [ ] Start refused without a mapping (MR-09).
- [ ] Mapping accepted while stopped, BUSY while following, cleared on reboot (MR-10, A8).
- [ ] Stop the broker mid-run: robot keeps driving, loss is shown, and recovery time is recorded (MR-33, A1, T13).
- [ ] Malformed, truncated and prohibited requests on the real link (T12, Gap 5). Host tests only so far.
- [ ] All six fault bytes under controlled fault injection (S1-M01).
- [ ] Rebuild the route from saved events (MR-38).
- [ ] Decide whether event drops need reporting; neither kind of drop appears in telemetry.
- [ ] Decide whether the professor's buffer wording requires literal RTOS message buffers (D09).

## 8. Faults seen and not explained

- [ ] Encoder faults stopped runs earlier, and the bad edges have no known cause. PR 149 stops the laptop showing speeds from bursty packet timing, such as `L=1935 R=-222` (INT-157 capture), but that does not rule out real encoder glitches; check against `WHEEL` frames.
- [x] Ten Pico restarts in INT-154 to INT-157: Nicholas identified a loose battery connection, a hardware fix. Excluded from software fault investigation; this does not establish a loaded-supply PASS.
- [ ] `Obstacle: no echo` appears in many runs. It is meant to be report-only; check it never causes a stop on its own.
- [ ] `IMU calibration failed` on one boot (INT-154 capture).
- [ ] `barcode capture overflow` is frequent. It no longer clears the runs; consider a larger ring if reads still fail.
- [ ] `EVENT BARCODE` missing for one read that telemetry shows (INT-154 capture). PR 149 adds a fallback event with no test; confirm on a real run that every read gives one event and no duplicates.

## 9. Full mission and repeatability

- [ ] Agree first-cut limits for lateral, turn, height, width and recovery error before any physical PASS (D10).
- [ ] Mission-day routine: fresh boot, broker, unfamiliar letters, local start, with no recompile (MR-35).
- [ ] Full course with line, barcodes, junctions, humps and an obstacle (MR-36, T16).
- [ ] Two more runs with no code or constant changes, for three in a row (MR-37). The brief assesses repeatability directly.
- [ ] Same junction revisited, repeated feature cycles, WiFi interrupted, GP21 ends the session (MR-40, endless operation).
- [ ] Run time compared with the roughly five-minute expectation (MR-36).

## 10. Week 13 evidence (minimum-bit, timing, resources)

- [ ] Timing on the final image under combined load: WCET, jitter, missed deadlines and sensor-to-motor latency (MR-39, T15, A11, D13). This is the evidence that timing has not degraded.
- [ ] No lost functionality: rerun the applicable set on the final commit (T15, T16).
- [ ] RAM usage of the final build; SYS-11 and SYS-12 are October builds, and the architecture guide still cites SYS-11 as final.
- [ ] Stack use and peak RAM measured (R3, D13). Never measured.
- [ ] Nicholas reviews the complete data table; remove stale per-person approval dependencies while retaining unmeasured bounds as provisional (Gap 1, R2).
- [ ] Nicholas confirms fault meanings and simultaneous-fault coverage; software writers retain their separate ownership (former #113 scope).
- [ ] Justify or reduce telemetry fault bytes: 48 bits are sent and 12 are defined.
- [ ] Add justification rows for stored floats, including `WheelPid`, or convert them to fixed point (Tribunal float audit).
- [ ] Add data table rows for the fields PR 149 added: two 32-bit watchdog heartbeats and the I/O task's last-fed copies.
- [ ] Update stale data table rows; some IR rows still describe the old ADC sensors.
- [ ] Decide whether obstacle profiles are quantized as said at Tribunal; they still store 16-bit millimetres.
- [ ] Height scale and rounding confirmed by reference humps; 16-bit working and 7-bit reported widths are in code (D06, Darrius's Tribunal item).
- [ ] Packed-data race checks on the Pico under load, including lock duration (R1, R4, Gap 2). Host stress only so far.
- [ ] Measure CPU cost of packing and unpacking (Tribunal resource review).
- [ ] Audit core-0 I/O integration and cross-core hand-offs (D14).
- [ ] Stale and frozen producers, expired commands and full buffers (T14).

## 11. Reports, demonstrations and traceability

- [ ] Link every result to its exact commit, image hash, setup and limit, with reruns kept (Gap 8, [testing](testing/README.md#run-and-record)).
- [ ] Record videos and fill the index; nothing is recorded yet ([video](video/README.md)).
- [ ] Integrated demonstration (Team deliverable).
- [ ] Telemetry demonstration (Comms deliverable).
- [ ] Motion accuracy evaluation, from MR-21 to MR-23.
- [ ] PID tuning report; only a tuning section exists in the Motion README.
- [ ] Terrain analysis report; no such file found.
- [ ] Record the Week 10 partial-integration result, if it was held. Its format is still pending in the specs, and no result is in the repository.
- [x] Issue #117 stays closed; live work and acceptance are tracked in this file. Active guide links now point here; historical evidence stays unchanged.
- [ ] Update the mission-readiness checklist as items pass.

## 12. Stretch goal

- [ ] Magnetometer and encoder heading fusion. The briefing marks fusion higher, but it is not required; no magnetometer code exists.

## 13. Housekeeping

- [ ] Nicholas confirms final diagnostic-frame retention (types 9, 10 and 11); reconcile the Comms README and record the chosen TRACE profile. Keep diagnostics available for investigation until that decision.
- [x] PR 149 is merged. Deleting its old branch is optional housekeeping, outside this task; do not block acceptance on it or delete another person's branch.
- [ ] Retain this file as the completion record, closing each item only with evidence or an explicit superseding decision.

## Complete

Software items are complete only as software; physical acceptance is listed above where it applies.

- [x] Uses micro T-Kernel, not FreeRTOS.
- [x] Only Motion writes the motors.
- [x] Networking cannot start, stop or drive the robot.
- [x] Course speed fixed at 100 mm/s by a compile-time check.
- [x] JSON replaced with binary frames in code and host tests; the real link is S1-M01 above.
- [x] Message sizes fixed and asserted: 23, 11, 5, 3 and 2 bytes.
- [x] Fault enums replaced with independent bits (Nicholas's Tribunal item).
- [x] One encoded value for each state field.
- [x] Scan state uses 3 bits (Jia Jing's Tribunal item).
- [x] Accessor functions for packed fields.
- [x] Mutex-protected snapshots written, with 5 million host reads and none torn (Shaun's Tribunal item); on-target check is in section 10.
- [x] Wire validation of length, version, type, reserved bits and ranges, in host tests.
- [x] Comms API, telemetry framework and frame documentation.
- [x] RTOS interfaces kept between subsystems.
- [x] `scripts/dashboard.html` and its test: the local edits were committed in #150.
- [x] PR 149 merged at `99fe0d8`, with its conflicts and three test-tool bugs fixed (INT-160, host only).
- [x] Merge commit `e576760` recorded for INT-132 to INT-159 (INT-161).
- [x] Watchdog code written, off by default; physical test is in section 1.
- [x] `TRACE=0` build, `scripts/mission_trial_score.py` and `scripts/triage_mqtt_capture.py` added, with host tests for the two scripts only; the laps are in section 3.
- [x] Laptop speed estimate ignores bursty packet timing, with a host test.
- [x] Integration range probe corrected to 20 bytes.
- [x] Readiness procedures updated for the digital IR sensors and reset flow, keeping MR-29 at 100 mm/s on humps.
- [x] PR 147 closed.
- [x] Overlapping logs trimmed; INT-135 and INT-136 are now 340 KB and 532 KB.

## Superseded

Closed by a later decision, so no work remains. `docs/tribunal1.md` still shows the earlier position.

- [x] Reset encoder distance at each barcode: replaced by wrapping counters with per-consumer origins (Gap 3).
- [x] Compare RAM and message sizes before and after: dropped by the course update of 2026-10-27.
- [x] Priority hierarchy unchanged with a separate Safety task: the 2026-10-29 design removed the task, and Motion still outranks Mission.
- [x] Finish convention (A10): endless operation is settled, with no finish marker. MR-34 stays retired.

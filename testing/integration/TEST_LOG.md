# Integration tests

Fresh definitions/results after the authorized reset. Previous results are recoverable only in Git and are not current PASS evidence. Firmware fixtures execute only in inhibited TEST; no host fake-kernel runner is used.

## Firmware cases

Each row below requires every grouped assertion to succeed. Expected PASS is a requirement, not an executed result. Fixture source: [mission_test.c](mission_test.c). Synthetic values are not hardware calibration.

| ID | Required behavior | Method |
| --- | --- | --- |
| INT-15 | valid complete settings accepted; missing headroom/course rejected | Inhibited firmware fixture |
| INT-16 | positive junction/finish and travel/action/dwell boundaries | Inhibited firmware fixture |
| INT-22 | distinct consecutive acquisitions, cache and wrap | Inhibited firmware fixture |
| INT-23 | braking begins outside center-based pivot envelope | Inhibited firmware fixture |
| INT-21 | any four letters, one per motion; 22 remain unmapped | Inhibited firmware fixture |
| INT-24 | start/scan resume rejects stale, missing, uncalibrated or impacted required producers | Inhibited firmware fixture |
| INT-25 | actual kernel ceiling raises effective priority and restores it after unlock; caller priority restored | Inhibited firmware fixture, no kernel mocks |

| INT-28 | Fresh matching-epoch quiet raw encoders plus stop/brake acknowledgement; reject stale, moving, faulted or unacknowledged | Inhibited firmware fixture |
| INT-29 | Shared hardware TIMER delay completes, zero delay accepted, excessive duration rejected | Actual inhibited firmware timer fixture |
| IT06 | Actual `mission_mapping_submit` accepts a valid request and rejects invalid, replay and pending-busy requests | Inhibited firmware fixture using the initialized T-Kernel mission mutex |

## Build and integrated cases

| ID | Required behavior | Method |
| --- | --- | --- |
| INT-17 | Four profiles, normal fixture exclusion, strict changed-code checks, source/image identity, unchanged main/video | Target compiler/build and repository audit |
| INT-18 | Atomic four-letter stopped replacement with live provider revision; busy/invalid/backpressure rejection and unchanged-image reboot/reload | Actual Pico and broker; PENDING |
| INT-19 | Combined calibration, leased actions, capture rearm, scan cancellation and original-route recovery | Actual Pico/controller/feature trials; PENDING |
| INT-20 | Known source defects corrected; regression definitions present; measured/course limitations explicitly retained | Source review and traceability; no physical proof |
| INT-27 | Six profiles including IR-only TEST; invalid combinations rejected and normal fixture exclusion retained | Build/profile audit |
| INT-26 | Mission/submission readiness excludes video but requires measured settings, required hardware, physical acceptance and reports | Repository/requirements audit; no physical PASS inferred |

## Manual integrated cases (PENDING)

Use the approved power/calibration sequence in [GUIDE](../../GUIDE.md), a normal mission image, a fresh console plus status/MQTT capture, and the confirmed course and settings. Record revision, run epoch, mapping ID/revision, action ID, sensor timestamps, encoder references, power/wiring, elapsed time and raw evidence. These cases have no accepted physical results yet; do not infer a pass from the inhibited fixture or a source/build check.

| ID | Steps | Pass oracle |
| --- | --- | --- |
| IT02 | With the approved four-letter mapping loaded, run the confirmed sequence for left, right, straight and U-turn. For each marker, capture the decoded symbol/action and run epoch; observe junction qualification, the resulting Motion action ID, encoder feedback, matching completion ACK, and the next navigation transition. Use an approved Motion-feedback fault-injection path to present a prior action ID after a newer action starts; if no approved injector is available, leave that clause pending rather than synthesizing a fake-kernel path. | Each symbol selects its configured action exactly once. Junction travel is qualified before motion starts. Both wheel targets qualify before `MOTION_DONE`; only the matching action ID can be acknowledged and advance navigation. A stale ID does not increment completed actions or release the next action. Record distance/angle error against confirmed tolerances. |
| IT03 | Begin from a fresh calibrated run. Press GP21 during a motion action, then repeat with GP21 during start and during a reset attempt. For each phase, capture button edges, stop flag, brake acknowledgement, run epoch, Motion feedback and output. Release GP21 and try GP20/start, old commands, and reset before and after the documented qualified reset sequence. | GP21 latches local stop and brakes within the approved stop budget. Release or GP20 cannot clear the latch or start motion. Old-epoch commands remain rejected; reset requires the matching brake acknowledgement and never starts a run. Record measured latency and stopping distance against the approved limits. |
| IT04 | Place the approved obstacle/course profile at its measured position. Observe detection and braking, then wait for fresh stationary feedback and brake acknowledgement before scan. Capture each sonar/servo sample and the selected profile. Run a permitted bypass through original-line reacquisition. Repeat with scan cancellation, health drift/stale feedback, and unknown or blocked clearance. | Pivot starts only after fresh stopped/stationary qualification and the approved scan profile permits it. The selected side and route stay inside confirmed course bounds; original line/pose/heading context is freshly reacquired. Cancellation, health loss, unknown clearance or blocked space keeps motion braked and cannot fall through to bypass. |
| IT05 | Record a previously accepted reference hump and its event/history snapshot. On the approved safe course, begin a second hump and produce the confirmed impact condition before its descent/baseline completes. Capture raw IMU, Motion, mission fault/stop, terrain validity, and history before and after impact. | Impact changes terrain/mission to the fault-stop path and obtains brake acknowledgement. The partial hump is invalid and never replaces last/max or appends an event; prior accepted history remains intact. Compare measured impact and reference-height observations only against approved thresholds. |

## Results

| Date | IDs | Revision / image SHA-256 | Profile/configuration | Tester | Result | Measurements / evidence |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-15 | INT-15, INT-16, INT-21, INT-22, INT-23, INT-24, INT-25, INT-28, INT-29 | `61bc484`; UF2 `22403565d19f666e49cc46fead2dd57c9fc8fb5a0f55b438497421164920f254` | USB-only, wireless IR-only TEST; missing IMU/sonar; motors inhibited | Nicholas / Codex capture | Firmware PASS | [Complete 45-check capture](logs/2026-10-15_134410_950691_pico-console.txt), [summary](logs/2026-10-15_134410_950691_pico-console.json); logic/timer/kernel checks only, no powered acceptance |
| 2026-10-15 | INT-17 | `c5e1832`; [image identities](logs/2026-10-15_fresh-acceptance_builds.json) | SMP=1, USB CDC, offline/wireless, TEST/normal; unconfirmed mission settings | Codex / automated tools | Software PASS | Four sequential builds; normal images exclude all six fixture entry points; [54 strict object checks](logs/2026-10-15_fresh-acceptance_strict.json); [resource estimates](logs/2026-10-15_fresh-acceptance_stack.json); [repository audit](logs/2026-10-15_fresh-acceptance_repository.json) |
| 2026-10-15 | INT-20 | `c5e1832`; no device image executed | Source review, portable encoder inspection and helper validation | Codex | Source reviewed; hardware PENDING | Corrections below; [11 portable encoding cases](logs/2026-10-15_fresh-release_codec.json) and [12 helper input cases](logs/2026-10-15_fresh-release_helper.json) PASS on laptop; source-file hashes identify these unchanged modules |
| 2026-10-15 | INT-17/20 | `c204e39`; working configuration differs in whitespace only, exact hash recorded | Four SMP USB CDC offline/wireless TEST/normal profiles; calibration/settings still unconfirmed | Codex | Software PASS | [Four builds/images](logs/2026-10-15_mission-hardening_builds.json), [62 strict target checks](logs/2026-10-15_mission-hardening_strict.json), [five static-analysis checks](logs/2026-10-15_mission-hardening_analysis.json), [frame estimates](logs/2026-10-15_mission-hardening_stack.json); 42 compiled fixture IDs, execution PENDING |
| 2026-10-15 | INT-26 | `c204e39`; source/settings and recorded evidence reviewed | Full mission requirements, video excluded | Codex | NOT READY | [Readiness audit](logs/2026-10-15_mission-readiness.json); hardware unavailable, measured/course settings unconfirmed, physical acceptance/report evidence pending |
| 2026-10-15 | IT06 | `bb4cdf9`; no firmware image executed | SMP=1, UART, TEST=1, Wi-Fi disabled; target object only | Codex | Target compile PASS; Pico execution PENDING | `mission_test.c` compiled with the ARM target object rule; this does not establish fixture execution or mapping API behavior on the Pico. |

Actual inhibited firmware execution is recorded above. Physical acceptance remains pending. Earlier compiled-only evidence retains its original status; builds never count as executed fixtures.

## Corrections and remaining gates

These rows trace the approved source fixes to focused checks. Firmware checks have executed on the USB-only Pico as recorded below. Physical scenario acceptance remains PENDING.

| Review item | Implemented behavior / verification owner |
| --- | --- |
| R1: braking/pivot geometry | [Mission guards](../../app_program/subsystem3/mission_guards.c) and [profile](../../app_program/subsystem5/obstacle_profile.c) include measured sonar offsets, swept car radius and braking headroom; INT-15/23, S5-U06, T08-T10 |
| R2: capture during short loss | [Mission](../../app_program/subsystem3/mission.c) resets/rearms capture, retains the qualified instruction and rejects another unresolved command; T04/T11 |
| R3: cached broad-line confirmation | [Dwell](../../app_program/subsystem3/mission_guards.c) requires distinct consecutive acquisition sequences/times; INT-22, T03/T11 |
| R4: turning suppressed by height filtering | [Movement processing](../../app_program/subsystem4/terrain_motion.c) separates pivot classification from hump travel acceptance; S4-U07, T06 |
| R5: old run height in WAIT/error snapshots | [Terrain reset](../../app_program/subsystem4/task_terrain.c) publishes a coherent cleared epoch/peak/history before acquisition; S4-U03/U04/U08, T07 |
| R6: lost mapping acknowledgement | [ACK queue](../../app_program/subsystem1/mapping_ack.c) reserves four result slots, retries QoS1 delivery and ignores obsolete callbacks without replaying configuration; S1-U10, INT-18, T12/T13 |
| R7: incomplete topic validation | All seven [network topics](../../lib/liblwip/sysdepend/pico_rp2040/mqtt_utk.c) reject wildcards/duplicates/invalid lengths; S1-U11, T12 |
| R8: cancel during center restoration | [Scan task](../../app_program/subsystem5/subsystem5.c) preserves cancellation through running, completed or failed restoration phases; S5-U05, T02/T09 |
| R9: previous-run search direction | Mission clears direction and visible/dwell observations at local reset and fresh run; T11 |
| G1: missed encounter reports | [Run histories](../../app_program/common/run_history.c) retain eight records per producer, count/eviction/saturation and new-run reset; S1-U12, S3-U16, T04/T07/T13 |
| G2: incomplete requested status | Schema-3 robot snapshots and cursor-based events use bounded encoders; S1-U04/U12/U13, portable encoding inspection, T12/T13 |
| G3: hidden obstacle depth / original route | Remains a course and physical acceptance gate. Sparse forward sonar cannot prove a passed rear edge or every possible route; T10/T11 and confirmed course bounds are required. |
| Joint black/IMU calibration | [Coordinator](../../app_program/subsystem3/task_s3.c) waits for both producers before one beep, ignores early white presses and cancels/fails at the bounded deadline; S3-U14/U17, S4-U05/U06, T01/T02 |
| Late GP21 / local reset | Mission preserves local-stop reason and rechecks inhibition around Motion reset and state publication; T02/T14/T15 |
| Health lost during scan | Required-data health and stopped/stationary/brake acknowledgement are rechecked before scan-to-bypass release; INT-24, T09/T14 |
| IMU acquisition failure/recovery | Calibration becomes invalid; stopped reinitialization preserves accepted last/max/history and requires fresh calibration; S4-U09/U10, T01/T07/T14 |
| Polling mutex preemption | Shared bounded command/snapshot locks use priority ceiling rather than relying on inheritance with TMO_POL; actual kernel fixture INT-25, measured hold/stop latency T15 |
| Non-finite wheel distances | Invalid origins are rejected and invalid distance feedback brakes the action; S2-U14, T05/T14 |

The strict object check at `8592a1b` found unused fixture helpers outside TEST; [that failed attempt](logs/2026-10-15_fresh-release_strict.json) is retained. The correction excludes those helpers and moves the large S1 test JSON buffer to TEST-only storage. [Final estimates](logs/2026-10-15_fresh-acceptance_stack.json) reduce its individual frame; runtime stack margin still requires T15. Earlier syntax-only checks did not expose this warning.

[Final evidence index](logs/2026-10-15_fresh-acceptance_evidence.json) identifies the accepted software revision, unchanged main/video, branch inclusion, source hashes and remaining gates. Earlier build attempts are retained for history; the `f971a0d` attempt included an uncommitted MQTT transport change later committed at `a212ae8`, so those images are not the final reference.

Physical coverage and procedures: [system cases](../system/TEST_LOG.md), [GUIDE](../../GUIDE.md), [testing policy](../README.md). Include source, image, power/wiring, calibration, mapping request/revision, reference/observed measurements, time and evidence for every real run.

## Available-hardware regression evidence

INT-17/27: `33ea475` passed [six sequential builds and normal fixture exclusion](logs/2026-10-15_available-release_builds.json) and [96 strict Arm object checks](logs/2026-10-15_available-release_strict.json). [Invalid-profile guards](logs/2026-10-15_available-hardware_guards.json) reject IR-only without TEST. Final image execution is recorded separately.

The 43-check `ffd4560` run completed; the 44-check `aadf6bd` raw capture contains every PASS and the end marker but its helper summary reports a later USB disconnect. The 45-check `61bc484` capture above completed cleanly. Earlier permission-denied, no-trigger, interrupted observation and disconnect summaries remain in `logs/`; they are not silently converted to PASS.

Live MQTT on `61bc484` accepted two different rehearsal mappings on unchanged firmware, rejected replay/duplicate-action/zero-ID requests, and showed advancing acquisition/reporting after the core-1 timer fix. Its 100 ms provider cache also exposed ages beyond 60 ms; `33ea475` reduces the cache period to 20 ms without relaxing freshness. See [S1 evidence](../subsystem1/TEST_LOG.md). The later unchanged-image reboot/reload subset is recorded below; full INT-18 transport stress and T13 network recovery remain pending.

Latest executed image: `33ea475`, UF2 `f554c1f1280098cfc6c3278992c0d5877f744dab0f07a1ef84d02e6b674318b0`; USB-only IR-only TEST, motors inhibited, tester Nicholas/Codex. **Firmware PASS** for INT-15, INT-16, INT-21, INT-22, INT-23, INT-24, INT-25, INT-28, INT-29; [complete 45-check capture](logs/2026-10-15_152409_309541_pico-console.txt), [summary](logs/2026-10-15_152409_309541_pico-console.json). Physical acceptance remains pending.

INT-18 reboot/reload subset PASS at `33ea475`: [normal reboot and reload](../subsystem1/logs/2026-10-15_available-release_reboot-reload.json) observed fresh revision 0 before reload and matching revision 1 acknowledgement afterward, with no recompile/reflash and motors inhibited. The [earlier BOOTSEL attempt](../subsystem1/logs/2026-10-15_available-release_reboot-attempt.json) is retained as incomplete. Full INT-18 busy/backpressure/moving transport checks and T12/T13 remain partial.

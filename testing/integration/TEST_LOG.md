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

## Build and integrated cases

| ID | Required behavior | Method |
| --- | --- | --- |
| INT-17 | Four profiles, normal fixture exclusion, strict changed-code checks, source/image identity, unchanged main/video | Target compiler/build and repository audit |
| INT-18 | Atomic four-letter stopped replacement with live provider revision; busy/invalid/backpressure rejection and unchanged-image reboot/reload | Actual Pico and broker; PENDING |
| INT-19 | Combined calibration, leased actions, capture rearm, scan cancellation and original-route recovery | Actual Pico/controller/feature trials; PENDING |
| INT-20 | Known source defects corrected; regression definitions present; measured/course limitations explicitly retained | Source review and traceability; no physical proof |

## Results

| Date | IDs | Revision / image SHA-256 | Profile/configuration | Tester | Result | Measurements / evidence |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-15 | INT-17 | `c5e1832`; [image identities](logs/2026-10-15_fresh-acceptance_builds.json) | SMP=1, USB CDC, offline/wireless, TEST/normal; unconfirmed mission settings | Codex / automated tools | Software PASS | Four sequential builds; normal images exclude all six fixture entry points; [54 strict object checks](logs/2026-10-15_fresh-acceptance_strict.json); [resource estimates](logs/2026-10-15_fresh-acceptance_stack.json); [repository audit](logs/2026-10-15_fresh-acceptance_repository.json) |
| 2026-10-15 | INT-20 | `c5e1832`; no device image executed | Source review, portable encoder inspection and helper validation | Codex | Source reviewed; hardware PENDING | Corrections below; [11 portable encoding cases](logs/2026-10-15_fresh-release_codec.json) and [12 helper input cases](logs/2026-10-15_fresh-release_helper.json) PASS on laptop; source-file hashes identify these unchanged modules |

Firmware execution: **PENDING**. Append actual complete console evidence only after execution. Builds/compiler checks belong in the integration log and do not fill this table with fixture PASS results.

## Corrections and remaining gates

These rows trace the approved source fixes to focused checks. Firmware check definitions compile; they have not yet executed on a Pico. Physical scenario results remain PENDING.

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

The strict object check at `8592a1b` found unused fixture helpers outside TEST; [that failed attempt](logs/2026-10-15_fresh-release_strict.json) is retained. The correction excludes those helpers and moves the large S1 test JSON buffer to TEST-only storage. [Final estimates](logs/2026-10-15_fresh-acceptance_stack.json) reduce its individual frame; runtime stack margin still requires T15. Earlier syntax-only checks did not expose this warning.

[Final evidence index](logs/2026-10-15_fresh-acceptance_evidence.json) identifies the accepted software revision, unchanged main/video, branch inclusion, source hashes and remaining gates. Earlier build attempts are retained for history; the `f971a0d` attempt included an uncommitted MQTT transport change later committed at `a212ae8`, so those images are not the final reference.

Physical coverage and procedures: [system cases](../system/TEST_LOG.md), [GUIDE](../../GUIDE.md), [testing policy](../README.md). Include source, image, power/wiring, calibration, mapping request/revision, reference/observed measurements, time and evidence for every real run.

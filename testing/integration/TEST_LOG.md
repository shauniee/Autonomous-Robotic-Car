# Integration test log

## Contents

- [Measured rolling-scale application — INT-67](#measured-rolling-scale-application--int-67)
- [Operator-assisted encoder calibration — INT-66](#operator-assisted-encoder-calibration--int-66)
- [User-reported encoder revolution count — INT-65](#user-reported-encoder-revolution-count--int-65)
- [Short physical sweep angle investigation — INT-64](#short-physical-sweep-angle-investigation--int-64)
- [500 ms recovery takeover qualification — INT-63](#500-ms-recovery-takeover-qualification--int-63)
- [Immediate simple 70 degree recovery — INT-62](#immediate-simple-70-degree-recovery--int-62)
- [Full-speed 25 mm straight junction exit — INT-61](#full-speed-25-mm-straight-junction-exit--int-61)
- [Navigation trigger and low-speed transition check — INT-60](#navigation-trigger-and-low-speed-transition-check--int-60)
- [Straight NAVIGATE slowdown and jitter audit — INT-59](#straight-navigate-slowdown-and-jitter-audit--int-59)
- [Wider repeating recovery sweep — INT-58](#wider-repeating-recovery-sweep--int-58)
- [Line tracking lifecycle audit — INT-57](#line-tracking-lifecycle-audit--int-57)
- [NAVIGATE jitter audit — INT-56](#navigate-jitter-audit--int-56)

- [Stable visible recovery handoff — INT-55](#stable-visible-recovery-handoff--int-55)

- [Recovery brake handoff fix — INT-54](#recovery-brake-handoff-fix--int-54)

- [Obstacle history — INT-53](#obstacle-history--int-53)

- [Recovery brake audit — INT-52](#recovery-brake-audit--int-52)

- [Retire the duplicate plan — INT-51](#retire-the-duplicate-plan--int-51)

- [Consolidated completion tracking — INT-50](#consolidated-completion-tracking--int-50)

- [Terrain header build compatibility — INT-48](#terrain-header-build-compatibility--int-48)
- [Barcode branch integration — INT-49](#barcode-branch-integration--int-49)

- [1. Test cases](#1-test-cases)
- [2. Results](#2-results)
- [Documentation alignment — 2026-10-29](#documentation-alignment--2026-10-29)
- [Documentation validation — INT-11](#documentation-validation--int-11)
- [Software restructure — 2026-10-29](#software-restructure--2026-10-29)
- [Docker tooling — INT-18](#docker-tooling--int-18)
- [PR integration gate — INT-19](#pr-integration-gate--int-19)
- [Repository findings — INT-20](#repository-findings--int-20)
- [Scan ID regression and cleanup — INT-21](#scan-id-regression-and-cleanup--int-21)
- [Completion record selection — INT-22](#completion-record-selection--int-22)
- [Rear-edge geometry — INT-23](#rear-edge-geometry--int-23)
- [Resource and binary-protocol audit — INT-24](#resource-and-binary-protocol-audit--int-24)
- [Event contention and unused-path cleanup — INT-25](#event-contention-and-unused-path-cleanup--int-25)
- [Recovery stop diagnostics — INT-26](#recovery-stop-diagnostics--int-26)
- [Recovery stall tuning — INT-27](#recovery-stall-tuning--int-27)
- [Sustained line-loss qualification — INT-28](#sustained-line-loss-qualification--int-28)
- [Recovery failure inputs — INT-29](#recovery-failure-inputs--int-29)
- [Forward FOLLOW slowdown — INT-30](#forward-follow-slowdown--int-30)
- [Recovery wheel-speed range — INT-31](#recovery-wheel-speed-range--int-31)
- [Stall configuration integration — INT-32](#stall-configuration-integration--int-32)
- [INT-33 — Wi-Fi startup diagnostic build — 2026-10-30](#int-33--wi-fi-startup-diagnostic-build--2026-10-30)
- [INT-34 — Diagnostic image startup observation — 2026-10-30](#int-34--diagnostic-image-startup-observation--2026-10-30)
- [Stationary wider recovery — INT-35](#stationary-wider-recovery--int-35)
- [One-screen dashboard — INT-41](#one-screen-dashboard--int-41)
- [Subsystem health table revision — INT-43](#subsystem-health-table-revision--int-43)
- [Toggleable binary steering buildup — INT-42](#toggleable-binary-steering-buildup--int-42)
- [Repository audit — INT-46](#repository-audit--int-46)
- [Repository audit fixes — INT-47](#repository-audit-fixes--int-47)

Template example INT-01/02 retired: S2-to-S3 sample flow (at least five in two seconds) and S3-to-S4 summary flow (at least two in 3.5 seconds). No template result was recorded. Logs refer to the uncommitted migration's source manifest plus base HEAD; they do not claim HEAD is the tested source. Formal on-target trials require a recorded committed revision.

Host methods below describe retained historical checks; their runners are retired. Firmware/build/manual methods remain applicable. See [testing procedure](../README.md).
- [Dashboard next-junction direction — INT-36](#dashboard-next-junction-direction--int-36)
- [Junction speed-controller reversal — INT-37](#junction-speed-controller-reversal--int-37)
- [Turn endpoint reactivation — INT-38](#turn-endpoint-reactivation--int-38)
- [Remove fixed junction advance — INT-39](#remove-fixed-junction-advance--int-39)

## 1. Test cases

| ID | Scope | Method | Expected |
|---|---|---|---|
| INT-03 | S3 request / S2 stop and expiry / health | Host actual Motion service with fake kernel, normal and TEST profiles | Stop independent of mutex; expired commands latch; reset cannot replay; TEST inhibits outputs |
| INT-04 | Reproducible template build and ownership | Normal and TEST=1 SMP builds; upstream byte comparison and static audit | Both UF2s; unchanged runtime/build source; init-before-start; native ADC disabled; no app network calls |
| INT-05 | Real S3 task and unchanged S2 API contract | Host task_ir with fake GPIO/ADC/S2, normal and TEST profiles | Recovery remains enabled; acquisition time preserved; GP21/reset/start, failed init/acquisition/health/publish/reset, inhibition and missed release fail closed |
| INT-06 | Read-only MQTT service | Actual mqtt_utk.c with fake lwIP/kernel/clock; 25 reconnect cycles | QoS0/nonretained; 500 ms stream; 3 s refusal/immediate-failure/alloc retry; skip unavailable/oversized/failed publication; latest reconnect; no timer accumulation |
| INT-07 | Telemetry isolation from S3 and S2 | Actual S3 loop with fake hardware/S2/kernel, MQTT enabled, normal and TEST; snapshot available or always busy | GP21 dominates; reset/start/recovery/watchdog and timestamps unchanged; failed telemetry never authorizes motion |
| INT-08 | Mac broker and terminal transport | Historical retired host runner, isolated loopback broker and CLI subscriber, actual JSON encoder fixture | Valid bounded JSON arrives; refusal/restart/reconnect; new subscriber gets no retained history; no Pico claim |
| INT-09 | Wireless/offline build and preservation | Normal/TEST MQTT and offline normal builds, source/Word/ownership audit | All UF2s compile; S2/control policy, kernel and historical sources preserved; credentials excluded |

| INT-10 | Restored S1 kernel reporting transport | Wireless TEST Pico run with broker/operator fault injection; pending | Status subscription before Online; 1 s heartbeat, retained online/offline will, authenticated broker; latest S3 schema unchanged; malformed/drive/start/stop/calibration requests rejected; status response rate bounded; 15 s attempt deadline and 1..30 s retry; link loss clears fragments, no stale replay or control calls; measure timer/stack budget |

## 2. Results

| Date | ID | Source | Tester | Result | Evidence | Notes |
|---|---|---|---|---|---|---|
| 2026-10-08 | INT-03 | logs/2026-10-08_source-sha256.json | Codex | PASS | logs/2026-10-08_host-checks.txt | Fake kernel, not scheduler/physical proof |
| 2026-10-08 | INT-04 | Same manifest; base HEAD in JSON | Codex | PASS | logs/2026-10-08_migration-evidence.json; logs/2026-10-08_build-normal.txt; logs/2026-10-08_build-test.txt | Both profiles compiled; no firmware flashing |
| 2026-10-08 | INT-03/05 | logs/2026-10-08_s3-proportional-source-sha256.json | Codex host run | PASS | [host log](logs/2026-10-08_s3-proportional-host-checks.txt) | Real S3 loop with fake inputs/S2 and unchanged real S2 service with fake kernel; both profiles |
| 2026-10-08 | INT-04 | Same manifest; base HEAD in JSON | Codex | PASS | [audit](logs/2026-10-08_s3-proportional-evidence.json); [normal build](logs/2026-10-08_s3-proportional-build-normal.txt); [TEST build](logs/2026-10-08_s3-proportional-build-test.txt) | Both UF2s; 564 protected files unchanged; no firmware flashing |
| 2026-10-08 | INT-03/05/06/07 | [source manifest](logs/2026-10-08_s3-mqtt-source-sha256.json) | Codex host run | PASS | [host log](logs/2026-10-08_s3-mqtt-host-checks.txt) | Fake kernel/hardware/network; unchanged S2 actual service also rerun in both profiles |
| 2026-10-08 | INT-08 | Initial pre-PTY host harness | Codex Mac run | FAIL | [initial harness log](logs/2026-10-08_s3-mqtt-broker-initial.txt) | Timed out waiting for pipe-buffered CLI debug output; fixed harness with terminal-style PTY; robot code unaffected |
| 2026-10-08 | INT-08 | Same manifest as final host run | Codex Mac run | PASS | [broker log](logs/2026-10-08_s3-mqtt-broker.txt) | Real Mosquitto loopback, synthetic C-encoded fixture; not firmware/Pico transport |
| 2026-10-08 | INT-09 | Same manifest and base HEAD | Codex | PASS | [audit](logs/2026-10-08_s3-mqtt-evidence.json); [normal MQTT build](logs/2026-10-08_s3-mqtt-build-normal.txt); [TEST MQTT build](logs/2026-10-08_s3-mqtt-build-test.txt); [offline build](logs/2026-10-08_s3-mqtt-build-offline.txt) | Built without flashing; private config content excluded; unrelated S3 formatting/comment edits left uncommitted |
| 2026-10-08 | INT-09 | Telemetry 57f3b95 plus main 6e8e31f documentation | Codex static audit | PASS | [post-sync audit](logs/2026-10-08_s3-mqtt-main-sync.json) | Integrated concurrent documentation update; 160 local links valid; runtime unchanged. Later user driver formatting is token-equivalent to the tested source; no new hardware claim |
| 2026-10-09 | INT-09 | [restoration manifest](logs/2026-10-09_s1-restore_evidence.json) | Codex build/static audit | PASS | [wireless TEST](logs/2026-10-09_s1-restore_wireless-test-build.txt), [wireless normal](logs/2026-10-09_s1-restore_wireless-normal-build.txt), [offline TEST](logs/2026-10-09_s1-restore_offline-test-build.txt), [offline normal](logs/2026-10-09_s1-restore_offline-normal-build.txt), [strict compile](logs/2026-10-09_s1-restore_strict-compile.txt) | S1 reporting/service restored; S2–S5, app startup/shared header, kernel and Make source unchanged; example credentials only; no flashing |
| 2026-10-09 | INT-10 | Same manifest | On-target run pending | PENDING | No Pico service log | Build and laptop-fixture checks do not execute kernel transport/retry/subscription on hardware |

## Documentation alignment — 2026-10-29

Planning note only; no test was executed and no historical result is changed.
The [central plan](../../plan.md) owns future implementation sequence;
[mission-readiness](../mission-readiness.md) owns target physical procedures.
Earlier task splits, GP20 reset flows, JSON messages, watchdog assumptions and
configured timing targets describe their recorded revisions. Do not interpret
them as target passes. Use the applicable procedure revision, exact source and
new evidence filenames for reruns; never overwrite evidence or reuse retired IDs.

## Documentation validation — INT-11

New case INT-11: documentation-only validation, 2026-10-29. Check every repository
Markdown contents list and local file/heading link, whitespace, and preservation
of historical test bodies. This case does not execute firmware or prove hardware.

| Date | ID | Source | Tester | Result | Evidence | Scope |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-29 | INT-11 | Base HEAD `4b00e20981c0eb7b2012f6dbeeacfb4cb5387730` plus [working-document SHA-256 manifest](logs/2026-10-29_INT-11_final-manifest.json); remote main verified as `5e6a45b768e7e2c2f8d69ba18c0360b6680d87de` | Codex | See validation log | [Check output](logs/2026-10-29_INT-11_final.txt), [checker](logs/2026-10-29_INT-11_checker.py) | Markdown/static only; no builds, firmware, flash or physical tests |

Initial attempts did not complete: Git object hydration timed out, followed by a
checker assumption about numbered versus unnumbered test headings. The final
checker handles both heading styles and uses an unstaged-index baseline. See
[initial method note](logs/2026-10-29_INT-11_initial.txt); only the final output
establishes the documentation-check result. No firmware claim is made.

## Software restructure — 2026-10-29

Case definitions follow the pre-existing [plan stages](../../plan.md#implementation-stages). These are software-only cases, not revised physical procedures or reused historical IDs. The source manifests identify the exact code; documentation drafts remain separate from the committed software. All profiles use SMP=1, USB CDC, CYW43, CRASH_REPORT=0, guards enabled, and the respective NET/MQTT/TEST settings. No firmware was flashed.

| New ID | Procedure / expected result |
| --- | --- |
| INT-12 | Stage #130: full kernel-free host suite plus offline/wireless normal/TEST compilation. Expect GP21/reset brake ordering, persistent expiry/action failure and no reset-induced movement in policy checks. Physical latency remains pending |
| INT-13 | Stage #131: same four builds and host suite, including seven states, once-only action IDs, stationary U-turn pause, cancellation, distinct outgoing acquisitions and bounded recovery. Scan/line work must contain no task sleep |
| INT-14 | Stage #132: same four builds; 100000 concurrent mapping request/reply exchanges, full event-post retry retention, optional IMU faults, frozen Terrain acquisition and invalid height windows. Actual kernel mutex/I2C behavior remains on-target work |
| INT-15 | Stage #133: final four builds and host suite; inspect three task creation sites, core/priority contracts, five mutexes and one flag, exclusive motor writes, no application networking, source/UF2 hashes and static linked sizes. Do not infer WCET, stack high-water, stopping distance or full mission success |

| Date | ID | Exact software commit | Result | Evidence |
| --- | --- | --- | --- | --- |
| 2026-10-29 | INT-12 | `fe39e3a5a9ef37a3a1205e7470e89e9e3a19dc30` | PASS (host/build only) | [Manifest](logs/2026-10-29_INT-12_stage1-manifest.json), [host](logs/2026-10-29_INT-12_stage1-host.txt), four corresponding build logs |
| 2026-10-29 | INT-13 | `b862817a40b7b75c7d2524621a1fffb5949a8f0f` | PASS (host/build only) | [Manifest](logs/2026-10-29_INT-13_stage2-manifest.json), [host](logs/2026-10-29_INT-13_stage2-host.txt), four corresponding build logs |
| 2026-10-29 | INT-14 | `ab54479a1b0044871cd6a6132c1747d6b3628299` | PASS (host/build only) | [Manifest](logs/2026-10-29_INT-14_stage3-manifest.json), [host](logs/2026-10-29_INT-14_stage3-host.txt), four corresponding build logs |

The first local Make attempt stalled on iCloud-generated build files. A fresh temporary build directory with the exact workspace sources, copied Make rules, official Arm GNU 13.2.Rel1 and Pico SDK 2.2.0 built successfully. Temporary mirror setup also required the project's `tools/` and `etc/` paths; those initial setup failures were not firmware results. The final wireless builds retain the upstream lwIP unused-function warning; application compilation is checked separately. Stage 2's first laptop self-test expected the old state-2 label FOLLOW; the updated seven-state decoder correctly calls state 2 NAVIGATE. The final rerun passes with unchanged golden bytes. Exploratory failures are kept separately from accepted per-stage evidence.

| 2026-10-29 | INT-15 | `407cbfa362a91db81706b255664a9425be0c33f6` | PASS (host/static/build only) | [Manifest](logs/2026-10-29_INT-15_stage4-manifest.json), [host](logs/2026-10-29_INT-15_stage4-final-host.txt), [strict application compile](logs/2026-10-29_INT-15_stage4-strict-app.txt), [allocation/image audit](logs/2026-10-29_INT-15_stage4-audit.json), four corresponding final build logs |

The final image has three application task symbols and five application mutex
creation sites plus one stop flag. Requested application stacks total 6144 bytes;
this is allocation, not stack high-water evidence. Wireless normal linked sections:
text 368840 bytes, data 392 bytes, BSS 84040 bytes (Arm GNU 13.2.Rel1). The audit
records every profile and built UF2 checksum; no flashed-image checksum or physical
result is claimed. [Exploratory logs](logs/2026-10-29_restructure-exploratory/) retain
setup/compile/review attempts; they are not source-manifested acceptance results.
Early host iterations also exposed unused-parameter and old-start-sequence test
assumptions, corrected before the recorded stage checks.

Continuation cases defined 2026-10-29 before accepted runs:

- INT-16: default production gates, 150 mm trigger, S3-U14/U15, S5-U03/U04 and full host regression; all four firmware profiles; strict application compilation and target ABI probes. Synthetic helper settings are fixture inputs, not enabled production calibration. Expect bounded software behavior, unchanged schema and pending scan/bypass release to brake.
- INT-17: audit the entire plan against actual evidence and current user decisions. Expected outcome while hardware is unavailable and D02/D05/D06/D07 remain pending: BLOCKED, never COMPLETE from software tests. Physical items remain unexecuted; this is an audit result, not a physical FAIL or PASS.

INT-16 default-gate fixtures also cover the newly confirmed 150 mm trigger and
the explicit `MISSION_BYPASS_RELEASED=0` inhibit. The separate legacy bypass
policy fixture compiles with `MISSION_BYPASS_RELEASED=1` only to exercise its
state transitions; that synthetic flag never enables a built firmware profile
or constitutes an accepted physical envelope/coverage release. Forward motion
in that policy now stops on missing/stale forward coverage. Full coverage
scheduling and rear-edge refinement remain unaccepted and blocked by the
deferred D02/D03/D05/D07 decisions.

| Date | ID | Source | Result | Evidence |
| --- | --- | --- | --- | --- |
| 2026-10-29 | INT-16 | Base `044889041aac67b78742f3b7004cd545755349fb` plus exact working-source manifest | PASS (host/static/build only) | [Manifest](logs/2026-10-29_INT-16-manifest.json), [host](logs/2026-10-29_INT-16-host.txt), four corresponding build logs, [strict compile](logs/2026-10-29_INT-16-strict.txt), [target ABI](logs/2026-10-29_INT-16-abi.txt) |
| 2026-10-29 | INT-17 | Same working-source manifest | BLOCKED (full-plan completion) | [All-stage audit](logs/2026-10-29_INT-17_completion.json), [checker output](logs/2026-10-29_INT-17_completion.txt) |

INT-16 uses explicit synthetic values inside the profile fixtures. Production
settings remain unknown/inhibited; the separate legacy bypass state-machine
fixture does not claim complete coverage or physical success. The full user
request is not marked complete: hardware trials are unavailable and the user
kept recovery/filter operating settings and Terrain new-run reset pending.

## Docker tooling — INT-18

Procedure defined 2026-10-29: validate Compose; build and start broker/dashboard;
verify broker health, publish/subscribe, HTTP access, mapping origin restrictions,
and dashboard subscription recovery after broker restart. Run decoder self-test
with read-only common headers and build wireless TEST firmware sequentially.
Expected: software checks pass and UF2 builds; no firmware execution or physical
robot pass is implied. No robot connected, wiring/power/calibration not applicable.
Base commit: `044889041aac67b78742f3b7004cd545755349fb`, with existing uncommitted
work. Results and source hashes will be appended after verification.

Results 2026-10-29, tester Codex, base revision above plus
[source manifest](logs/2026-10-29_INT-18-manifest.json):

- PASS: Compose configuration validation, both image builds, broker health,
  decoder self-test, MQTT round trip, HTTP page, invalid-origin rejection (403),
  same-origin invalid-mapping rejection (400), and dashboard MQTT stream.
  [Runtime evidence](logs/2026-10-29_INT-18-runtime.txt),
  [probe](logs/2026-10-29_INT-18_probe.py).
- PASS: same transport/stream checks after broker restart, without restarting
  dashboard. [Reconnect evidence](logs/2026-10-29_INT-18-reconnect.txt).
- PASS: Mac host HTTP 200 at 127.0.0.1:8080.
  [Host evidence](logs/2026-10-29_INT-18-host-http.txt),
  [services and image IDs](logs/2026-10-29_INT-18-services.txt).
- FAIL: wireless TEST firmware compilation encounters undefined
  `BYPASS_REACQUIRE` in existing `app_program/mission/mission.c:67`.
  [Build evidence](logs/2026-10-29_INT-18-firmware.txt).
  No new UF2 success or physical/firmware execution pass claimed.

Runtime startup was initially delayed in Docker Desktop, then completed without
configuration changes. Transport probes used synthetic payloads, no retained
messages or robot commands. Broker and dashboard remain running for the user.

## PR integration gate — INT-19

2026-10-29, Codex. Before publishing all working changes, run
`sh testing/pure/run.sh`; expected all host regressions compile and pass.
Base revision `044889041aac67b78742f3b7004cd545755349fb` plus uncommitted
source captured by [manifest](logs/2026-10-29_INT-19-manifest.json).
Result: FAIL at host compilation; tests still reference removed Mission bypass
fields/enumerators. [Initial output](logs/2026-10-29_INT-19-host-initial.txt).
The earlier INT-18 firmware build independently failed on a removed bypass
field/enumerator in mission.c. No physical execution or hardware pass claimed.

Follow-up results 2026-10-29, Codex, source commit
`bedc45fc560cb24a46f0c8ad03f5d17a2d6a90d2`:

- PASS: complete macOS host suite after integration fixes, including both bypass
  routes with encoder/sensor evidence, three-sample confirmation, scan retry/ID
  checks, stale-front and failed-action stops, and production release gates.
  [macOS output](logs/2026-10-29_INT-19-macos-host-final.txt).
- PASS: complete Linux GCC host suite and Python decoder self-test inside the
  rebuilt firmware container. Python was added to that image because the first
  Linux run passed C tests but could not launch the Python self-test.
  [Final Linux output](logs/2026-10-29_INT-19-linux-host-final.txt),
  [first Linux output](logs/2026-10-29_INT-19-linux-host.txt).
- PASS: Cortex-M0+ Motion size probe and all four offline/wireless TEST/normal
  firmware builds. [Build output and UF2 hashes](logs/2026-10-29_INT-19-all-profiles.txt).
  The pre-existing generated Linux converter initially returned I/O error;
  rebuilding only that ignored tool resolved it. Firmware compilation itself
  had already passed after the Mission field correction.
- GitHub Actions did not execute: repository account billing/spending limits
  prevented runner startup. [Annotation](logs/2026-10-29_INT-19-ci-block.txt).
  Local checks cover workflow commands; they are not a hosted-CI pass.
- [Final source manifest](logs/2026-10-29_INT-19-final-manifest.json) records the
  committed source tested. No flashed image, physical acceptance, calibrated
  limit, or release-gate change is claimed. Earlier failure outputs remain intact.


## Repository findings — INT-20

Procedure defined 2026-10-30 before validation: rerun the kernel-free host suite,
including 25 full-ring link-loss/reconnect cycles (both broker-only and Wi-Fi
loss). Expect no queued frames after reconnect, offline posts rejected and fresh
online frames retained. Inspect Mission adapter scan ID, profile geometry,
range timestamps/sequences and encoder-count wiring. Build all four documented
firmware profiles sequentially with SMP=1, USB CDC, CYW43 and CRASH_REPORT=0;
expect successful linking and UF2 conversion. Preserve source hashes and image
hashes. Run the completion audit against new evidence; expect current software
evidence with physical stages still BLOCKED/PARTIAL, never physical acceptance.

Interface revision: range samples now retain the wrapped mean encoder count at
front-range completion for bypass travel accounting; storage is documented in
[project-specs](../../docs/project-specs.md). S1 rejects offline TX posts and the
transport flushes TX on loss/reconnection. Removed unused range-word getters and
event-drop getter/counters; no telemetry wire format change.

| Date | ID | Source | Result | Evidence |
| --- | --- | --- | --- | --- |
| 2026-10-30 | INT-20 | Base `62ee5226e16a3c501d94998291944753c9662dfc` plus exact working-tree manifest | PASS (host/build/static); full-plan BLOCKED | [Manifest](logs/2026-10-30_INT-20-manifest.json), [host suite](logs/2026-10-30_INT-20-host-verified.txt), four `verified-build` logs, [profile image hashes](logs/2026-10-30_INT-20-profile-images.json), [completion audit](logs/2026-10-30_INT-20-completion.json) |

Native Arm GNU 13.2.Rel1 and Pico SDK 2.2.0; no flash or physical trial. Host
suite includes reconnect checks, 5,000,000 coherent word reads and 300,000 frames
with zero tears/corruptions/order failures. All four named profile images linked
and converted to UF2. The profile-image JSON identifies only these four outputs;
an old iCloud duplicate (`test 2.uf2`) in wildcard listings is not a tested image.
Earlier local run/build outputs are preserved under their distinct filenames.
Existing application signedness and upstream lwIP unused-function warnings remain;
no build errors. Static review confirms all Mission input fields are supplied,
scan state/ID share one snapshot, requests use output IDs, and removed APIs have
no remaining active references. Source is uncommitted; base HEAD alone does not
identify the tested tree. Historical INT-17 evidence is preserved.

## Scan ID regression and cleanup — INT-21

Procedure defined 2026-10-30: run `sh testing/pure/run.sh`, then build offline
and wireless, normal and TEST profiles sequentially using Arm GNU 13.2.Rel1
and Pico SDK 2.2.0. Expected: all host checks pass, scan IDs 0..15 round-trip
through the shared publisher packing and Mission reader for every scan state,
all four profiles link, and the bypass depth signedness warning is absent.
Check removed `s5_profile_copy` has no active source/documentation references.
No wiring, power or calibration applies; no firmware execution or physical trial.

| Date | ID | Source | Tester | Result | Evidence |
| --- | --- | --- | --- | --- | --- |
| 2026-10-30 | INT-21 | Base `62ee5226e16a3c501d94998291944753c9662dfc` plus working-source hashes | Codex | PASS (host/build only) | [Manifest](logs/2026-10-30_INT-21-manifest.json), [host](logs/2026-10-30_INT-21-host.txt), [offline TEST](logs/2026-10-30_INT-21-build-offline-test.txt), [offline normal](logs/2026-10-30_INT-21-build-offline-normal.txt), [wireless TEST](logs/2026-10-30_INT-21-build-wireless-test.txt), [wireless normal](logs/2026-10-30_INT-21-build-wireless-normal.txt), [image hashes](logs/2026-10-30_INT-21-images.json) |

All host tests passed with warnings treated as errors, including scan ID/state
round trips for all 128 combinations and isolation from reserved high bits.
All four firmware profiles linked and produced UF2 images. The bypass depth
signedness warning is absent; wireless builds retain only the upstream lwIP
`mqtt_msg_type_to_str` unused-function warning. Static search found no remaining
`s5_profile_copy` references in application code or subsystem documentation.
No firmware was flashed or executed; these results do not establish physical
obstacle or bypass behavior. The manifest identifies the uncommitted source;
base HEAD alone does not identify the tested tree.

## Completion record selection — INT-22

Procedure: run `python3 testing/check_plan_completion.py` after adding the INT-21
completion record. Expect selection of the newest dated completion JSON, current
source hashes and exit code 1 for the existing BLOCKED physical stages, rather
than exit code 2 for stale evidence. No wiring, power or calibration applies.

The appended [INT-21 completion record](logs/2026-10-30_INT-21-completion.json)
references the existing INT-21 manifest, host/build logs and image hashes. Stage
assessments remain unchanged from INT-20; no new physical acceptance is claimed.

| Date | ID | Source | Tester | Result | Evidence |
| --- | --- | --- | --- | --- | --- |
| 2026-10-30 | INT-22 | Base `62ee5226e16a3c501d94998291944753c9662dfc` plus checker SHA-256 in evidence; INT-21 manifest identifies application source | Codex | PASS (completion checker only); full-plan BLOCKED | [Checker output and source hash](logs/2026-10-30_INT-22-completion-check.txt) |

All 161 INT-21 manifest entries match the working tree. The checker selects
INT-21 automatically, reports current software evidence and exits 1 as expected.
Historical evidence is preserved. No firmware was built, flashed or executed in
this validation.

## Rear-edge geometry — INT-23

Procedure defined 2026-10-30: run `sh testing/pure/run.sh` with warnings treated
as errors. Expect both bypass-side fixtures to report depth 328 +/-2 mm from
the first qualifying side observation (~300 mm forward), the proposed 100 mm
sonar mount, 300 mm baseline at 75 degrees and 150 mm profile front. Confirm
the full bypass and recovery regressions pass with shared encoder heading and
front-clearance helpers. Capture source SHA-256 hashes and base HEAD before
execution. No wiring, power or calibration applies; this is host-only evidence.

| Date | ID | Source | Tester | Result | Evidence |
| --- | --- | --- | --- | --- | --- |
| 2026-10-30 | INT-23 | Base `62ee5226e16a3c501d94998291944753c9662dfc` plus exact working-source hashes | Codex | PASS (host only) | [Manifest](logs/2026-10-30_INT-23-manifest.json), [host suite](logs/2026-10-30_INT-23-host.txt) |

All host checks passed, including the expected depth assertion on both bypass
sides, full return/reacquisition sequences and existing recovery checks.
`git diff --check` passed. No firmware build, flash or physical trial was run.
Previous completion/build evidence remains historical and does not identify
these changed sources; this case records only the host suite on the new source.

## Resource and binary-protocol audit — INT-24

Procedure defined 2026-10-30 before the final run: run the host suite with
warnings as errors, including 30000 PID samples versus the previous float
controller (<=1 Q7 count difference), reset/invalid-history cases, six-byte
fault walking bits, and no-echo tracking versus bypass policy. Build all four
current profiles sequentially using Arm GNU 13.2.Rel1 / Pico SDK 2.2.0.
Probe all subsystem layouts plus current integration snapshots on Cortex-M0+;
record linked text/data/bss, allocated RAM (data+bss), object symbol sizes,
inter-task copy sizes and image hashes. Capture base HEAD and source hashes.
Expected: software passes, matching ABI assertions, unchanged wire sizes
23/11/5/3/2 bytes. No physical, WCET, stack high-water or T15 pass is inferred.
No hardware, wiring, power or calibration applies to these host/build checks.

| Date | ID | Source | Tester | Result | Evidence |
| --- | --- | --- | --- | --- | --- |
| 2026-10-30 | INT-24 / SYS-12 | Base `62ee5226e16a3c501d94998291944753c9662dfc` plus exact working-source hashes | Codex | PASS (host/build/static); full-plan BLOCKED | [Manifest](logs/2026-10-30_INT-24-accepted-manifest.json), [host](logs/2026-10-30_INT-24-accepted-host.txt), [ABI](logs/2026-10-30_INT-24-accepted-abi.txt), [resource cross-check](logs/2026-10-30_INT-24-accepted-resources.json); four `accepted-build` logs and per-profile `accepted-images` hashes |

All four builds pass; wireless profiles retain the upstream lwIP unused-function
warning, with no application warnings. The PID comparison covers 30000 samples:
constant +/- full error gives zero Q7 difference; varying error gives maximum
one count. State transitions, turn actions, same-sample hold, invalid-history
reset, both bypass sides and no-echo moving-offset stop regressions pass.
S1-U09..U13 replace the retired JSON acceptance cases; TEST startup execution
and binary Pico transport remain pending. SYS-12 owns the size interpretation.

Exploratory outputs are preserved: `host-initial` failed compilation on a
misspelled enum identifier (corrected to BARCODE_U_TURN); `host` used an invalid
fixture assumption that an unknown reading while already stopped for sensing
immediately changes Mission state. The corrected fixture injects echo loss
during an active offset and confirms STOP. `final-host` passes; `final-abi`
stopped on removed S3 steering fields in the old probe. The probe now measures
only current fields. These earlier outputs are not accepted passes. Current
accepted source hashes were verified unchanged after the build/probe checks.

No firmware/physical run or T15 was executed. Final accepted-commit resource
and minimum-bit review, live memory/timing and mission trials remain pending.

## Event contention and unused-path cleanup — INT-25

Procedure defined 2026-10-30 before execution: run the kernel-free host suite,
including the real `mission_mapping` hand-off and `profile_edges` estimator;
remove tests for deleted `s1_handle_command`, `s5_profile_build` and
`s5_fine_angles`. Build the four offline/wireless normal/TEST profiles using
Arm GNU 13.2.Rel1 and Pico SDK 2.2.0. Run current target size probes and save
linked size/symbol/image hashes. Verify no references to deleted APIs remain,
`lock_misses` occupies two 32-bit words, insertion uses the 2 ms bounded lock,
and no M0+ atomic RMW helper is introduced. Expect host/build/static checks to
pass and binary frames unchanged. Actual contended-lock timeout/increment and
saturation on the dual-core target require an on-target fault-injection trial;
host tests do not execute `subsystem1.c` or prove scheduler timing.

| Date | ID | Source | Tester | Result | Evidence |
| --- | --- | --- | --- | --- | --- |
| 2026-10-30 | INT-25 | Base `62ee5226e16a3c501d94998291944753c9662dfc` plus working-source hashes | Codex | PASS (host/build/static); full-plan BLOCKED | [Manifest](logs/2026-10-30_INT-25-accepted-manifest.json), [host](logs/2026-10-30_INT-25-accepted-host.txt), [static](logs/2026-10-30_INT-25-accepted-static.txt), [ABI](logs/2026-10-30_INT-25-accepted-abi.txt), [resources](logs/2026-10-30_INT-25-accepted-resources.json); four `accepted-build` logs and per-profile `accepted-images` SHA-256 files |

The host suite and all four builds pass; wireless profiles retain only the
upstream lwIP unused-function warning. Active Mission mapping-hand-off and
profile-edge tests pass. Deleted APIs/types are absent from application and
host-test sources. The target object retains the 8-byte `lock_misses` array
and has no atomic helper dependency; 25 persistent object size cross-checks
pass. Per-core counters use only load/store, with no shared RMW. Event insertion
requests the existing 2 ms timeout; I/O posting keeps TMO_POL and retains the
oldest event on bridge failure. BUSY no longer implies motion.

Normal static RAM is now 26840 bytes offline and 84520 bytes wireless, an
8-byte increase from INT-24 for the two diagnostic words. MQTT sizes remain
23/11/5/3/2 bytes; no new telemetry field. No firmware was executed/flashed.
Host coverage exercises `mission_mapping`, not the kernel-bound `s1_step`
transport dispatch; target lock-contention/saturation and timing remain pending.

## Recovery stop diagnostics — INT-26

Recorded after execution, 2026-10-30: wireless normal build with `TEST=0`,
`NOGUARDS=0` and the working-tree recovery release enabled. Added console
reporting of actual safety cause bits, transient Motion stall details after
braking, owner faults and producer ages. No navigation thresholds changed.

| Date | ID | Source | Tester | Result | Evidence |
| --- | --- | --- | --- | --- | --- |
| 2026-10-30 | INT-26 | Base `cc3edb3dd7c92ec76fb2be742171818095fdc49f` plus working-diff/source hashes | Codex | PASS (build only); hardware diagnosis pending | [Build and observations](logs/2026-10-30_INT-26-diagnostics-build.json) |

Build exit 0; UF2 SHA-256 and source hashes are in the evidence. The new
image was not flashed. The connected robot reported REACQUIRE/RUNNING,
encoder changes and then STOPPED with a safety latch. Its flashed revision
is unknown, so these observations are not a firmware/physical pass for this
build. The old console only prints latch truth, and 500 ms telemetry may
miss the stall fault cleared by Motion after braking. Existing Terrain
calibration-failed and Obstacle no-echo reports do not identify this stop.
A repeat using the new diagnostic image is needed before attributing the
stop to stall, a producer deadline or another safety cause.

## Recovery stall tuning — INT-27

Procedure defined before software checks, 2026-10-30: run the existing
kernel-free Mission tests with recovery/bypass explicitly enabled, and
Mission pending-policy tests with both explicitly disabled. Build the
wireless normal (`TEST=0`, `NOGUARDS=0`) firmware. Expect bounded recovery
and existing stop policy to pass; no physical recovery pass is implied.
The new standalone recovery speed is a 90 mm/s trial proposal after two
console-reported motor stalls at 30 mm/s with steering -96. Bypass line
alignment retains 30 mm/s under its own constant. New static checks bound
the outer-wheel target for both commands.

Observed on the connected robot before the change (flashed revision not
independently verified; user reports flashing INT-26 image):

```text
[MISSION] state=REACQUIRE
[MOTION] motor stall: mode=1 action=1 speed_mmps=30 steering_q=-96 left=96 right=157
[SAFETY] causes=2 gp21=0 fault=1 line_stale=0 motion_stale=0 motion_faults=1 terrain_faults=2 line_age_ms=0 motion_age_ms=10
[MISSION] state=STOPPED
[MISSION] stop: gp21=0 latched=2 inputs_ok=1 motion_stopped=1 motion_status=0
```

A second attempt reported the same stall command with left=253, right=416
and the same safety cause/fault bytes. Neither attempt establishes which
wheel stalled or a calibrated minimum wheel speed. The proposed speed
requests approximately 22/158 mm/s at full search steering, versus 7/53
previously. A repeat physical run is needed to establish whether the higher
command prevents stall and completes the sweep/reacquisition.

| Date | ID | Source | Tester | Result | Evidence |
| --- | --- | --- | --- | --- | --- |
| 2026-10-30 | INT-27 | Base `cc3edb3dd7c92ec76fb2be742171818095fdc49f` plus source/diff hashes | Codex | PASS (two host binaries and wireless normal build); physical recovery pending | [Manifest](logs/2026-10-30_INT-27-manifest.json), [host](logs/2026-10-30_INT-27-host.txt), [build](logs/2026-10-30_INT-27-build.txt) |

The compiler reported no warnings in this incremental build. No image
was flashed by Codex and no recovery pass is claimed at the new speed.
The new minimum/outer-wheel limits remain uncalibrated. Firmware stall,
stop-button, stale-input, time, travel and heading guards remain enabled.

## Sustained line-loss qualification — INT-28

Procedure defined before execution, 2026-10-30: run the complete existing
host suite with new Mission regression scenarios for the user-requested
continuous white/white interval. Check first loss, exactly 1000 ms (still
FOLLOW_LINE), more than 1000 ms (REACQUIRE), holding tracking steering/speed
with original acquisition freshness, visible-line reset, acquisition-gap
reset, stale cached input stop, local stop, zero timestamp and wraparound.
Retain bounded recovery/alignment tests. Explicitly compile disabled-policy
fixtures with both release flags zero regardless of the selected firmware
configuration. Build wireless normal firmware with TEST=0/NOGUARDS=0;
record source/image hashes. Expected: host/build PASS, no changed motor
ownership, stop/freshness policy, barcode routing or bypass behavior.
Physical tracking and one-second loss timing require a subsequent trial.

| Date | ID | Source | Tester | Result | Evidence |
| --- | --- | --- | --- | --- | --- |
| 2026-10-30 | INT-28 | Base `cc3edb3dd7c92ec76fb2be742171818095fdc49f` plus source/diff hashes | Codex | PASS (full host suite and wireless normal build); physical run pending | [Manifest](logs/2026-10-30_INT-28-manifest.json), [host](logs/2026-10-30_INT-28-host.txt), [build](logs/2026-10-30_INT-28-build.txt) |

All host scenarios and decoder/stress checks pass, including the strict
more-than-one-second boundary and wraparound. Build exit 0 with no warnings;
linked text/data/BSS are 377796/396/84132 bytes. The new timestamps are
Mission-local stack state, with one pending bit packed into existing flags.
No physical timing, tracking, recovery or stack-headroom pass is claimed.
Codex did not flash or command the robot.

## Recovery failure inputs — INT-29

2026-10-30 connected-console observation: several attempts show
FOLLOW_LINE -> REACQUIRE -> STOPPED, with `gp21=0 latched=0 inputs_ok=1
motion_stopped=0 motion_status=1`. This differs from the earlier motor
stall. It indicates Mission requested the stop, but the old log does not
identify which recovery precondition/bound failed. Flashed source/image
was not independently verified; no physical pass is claimed.

Procedure defined before build: compile wireless normal firmware with
TEST=0/NOGUARDS=0 after adding an exceptional recovery-failure console
snapshot. Expect build PASS and no control-policy changes. Diagnostics
run after the STOP request and command, and print Mission-local copies of
original failure inputs: elapsed time, signed encoder travel, heading,
mean travel, leg, IR usability/age and front obstacle validity/age/range.
No new persistent fields or wire data. Actual failure attribution needs a
repeat capture using this image; do not infer a cause from old logs.

| Date | ID | Source | Tester | Result | Evidence |
| --- | --- | --- | --- | --- | --- |
| 2026-10-30 | INT-29 | Base `cc3edb3dd7c92ec76fb2be742171818095fdc49f` plus source/diff hashes | Codex | PASS (build only); physical failure attribution pending | [Manifest](logs/2026-10-30_INT-29-manifest.json), [build](logs/2026-10-30_INT-29-build.txt) |

Build exit 0; linked text/data/BSS 378200/396/84132 bytes. No image
flashed by Codex. Existing INT-28 host evidence covers unchanged control
policy; no new host behavior test was added for console-only diagnostics.

## Forward FOLLOW slowdown — INT-30

2026-10-30 connected console reports two REACQUIRE failures after 40 ms:
left_counts=-25/right_counts=2 and left_counts=-24/right_counts=0;
heading_cdeg=675/600, travel_mm=-6, leg=0, usable_ir=1, ir_age_ms=0.
Both stop records have gp21=0/latched=0/inputs_ok=1/motion_stopped=0/
motion_status=1. Reverse signed encoder travel triggers the recovery
guard. User-reported diagnostic image identity was not independently read
back. This does not prove hardware encoder polarity or identify PWM at
the physical failure.

Procedure before execution: reproduce the tracking-to-recovery slowdown
with the actual Motion controller, unchanged FOLLOW action ID, and fresh
positive encoder travel from the preceding faster release. Expect the new
forward-output assertion to fail before the fix; save the actual duty.
Then constrain FOLLOW to forward/zero output with saturation-aware integral
updates, preserving signed DISTANCE/TURN control. Run the full host suite
(including reverse-distance and pivot tests) and build wireless normal
firmware TEST=0/NOGUARDS=0. Preserve one-second loss qualification and all
recovery stop limits. Physical movement/polarity and recovery remain
pending until a subsequent measured run.

| Date | ID | Source | Tester | Result | Evidence |
| --- | --- | --- | --- | --- | --- |
| 2026-10-30 | INT-30 | Base `cc3edb3dd7c92ec76fb2be742171818095fdc49f` plus source/diff hashes | Codex | Regression reproduced; PASS (full host suite and build); physical fix pending | [Manifest](logs/2026-10-30_INT-30-manifest.json), [before](logs/2026-10-30_INT-30-red-host.txt), [after](logs/2026-10-30_INT-30-host.txt), [build](logs/2026-10-30_INT-30-build.txt) |

Before fix the actual controller produced left duty=-0.116496 and right
duty=0.128173; the nonnegative FOLLOW assertion failed (exit -6). After
fix the same trace gives left=0/right=0.128173, then resumes positive
output without negative integral windup. Full host suite passes, including
reverse DISTANCE, pivot TURN, stall/expiry, one-second line-loss qualification
and recovery bounds. Wireless normal build exits 0 with no warnings;
text/data/BSS 378232/396/84132 bytes. No firmware flashed by Codex and no
physical recovery/polarity pass. The reproduced software reversal is a
mechanism consistent with the encoder failure, not proof of physical PWM
or encoder polarity during the observed robot run.

## Recovery wheel-speed range — INT-31

2026-10-30 connected console: FOLLOW_LINE -> REACQUIRE, then
`[MOTION] motor stall: mode=1 action=1 speed_mmps=90 steering_q=-96 left=2214 right=1583`.
Safety causes=2, motion_faults=1, line_stale=0, motion_stale=0,
line_age_ms=0, motion_age_ms=10. The delay did enter recovery; the search
then stalled. Running-image identity and which wheel stalled were not
independently established. No physical success claimed.

Procedure before checks: run the full host suite with a regression
requiring the recovery inner-wheel target to reach the normal maximum-
steering tracking range. Add a four-leg forward-only encoder trajectory
with +/-71-degree endpoint samples (within target tolerance), encoder wrap,
and failure beyond 75 degrees or after four legs. Preserve the diagnosed
90 mm/s/-96 reversal fixture independent of current tuning. Build wireless
normal TEST=0/NOGUARDS=0. Expect host/build PASS and the one-second trigger
and safety guards unchanged. The 270 mm/s/76 steering and 1200 mm budget
are trial settings, not measured minimum speeds or proven recovery motion.
Physical confirmation remains pending.

| Date | ID | Source | Tester | Result | Evidence |
| --- | --- | --- | --- | --- | --- |
| 2026-10-30 | INT-31 | Base `cc3edb3dd7c92ec76fb2be742171818095fdc49f` plus source/diff hashes | Codex | PASS (full host suite and build); physical sweep pending | [Manifest](logs/2026-10-30_INT-31-manifest.json), [host](logs/2026-10-30_INT-31-host.txt), [build](logs/2026-10-30_INT-31-build.txt) |

Full host suite and wireless normal build pass. Synthetic sweep completes
all four forward-travel legs within the revised budget and stops after
exhaustion; 71-degree samples reverse while 76 degrees fails. The test
uses synthetic encoder positions, not a calibrated moving plant. The
original one-second qualification and 90/-96 reverse-power regression
still pass. Build has no warnings; text/data/BSS 378236/396/84132 bytes.
The new image was not flashed by Codex. The selected wheel-speed range
and larger budget are proposals requiring physical verification.


## Stall configuration integration — INT-32

2026-10-30: Nicholas confirmed the pending default-disabled motor-stall guard
is intentional and requested updated documentation/tests before publication.
The first host-suite attempt failed compilation because the guard definition
was nested inside the recovery override conditional. Give the guard its own
override conditional. Before rerun: require the full host suite to cover the
default disabled guard (stuck wheel continues until command expiry) and an
explicit enabled guard (stuck wheel fails and brakes). Existing local stop,
command expiry, action deadlines and recovery bounds must remain covered.
Build all four standard firmware profiles sequentially. Record precommit source
hashes and base revision; these checks do not establish physical acceptance.

Host result: PASS, including both stall configurations. The initial added
expiry assertion ran before expiry and failed; its output is preserved in
[red host output](logs/2026-10-30_INT-32-red-host.txt). Advancing the simulated
clock beyond expiry gives the expected brake/failure. Accepted output:
[host suite](logs/2026-10-30_INT-32-host.txt),
[source manifest](logs/2026-10-30_INT-32-manifest.json), base
`cc3edb3dd7c92ec76fb2be742171818095fdc49f`. Tester: Codex. Build verification
follows integration with current main; no physical result claimed.

Integration result after rebasing onto main `dc67845`: PASS host suite and
all four standard firmware builds, NOGUARDS=0. Exact tested commit:
`e66836812c85efc5872a955995e66c5c005ca362`. Two upstream lwIP unused-function warnings (one in
each wireless build), no build errors. Preserved evidence:
[integrated host](logs/2026-10-30_INT-32-integrated-host.txt),
[build](logs/2026-10-30_INT-32-build.txt),
[image checksums](logs/2026-10-30_INT-32-integrated-manifest.json).
Tester: Codex, 2026-10-30. No firmware flashed and no physical pass claimed.

## INT-33 — Wi-Fi startup diagnostic build — 2026-10-30

Case: build wireless motor-inhibited TEST firmware with capped radio delay and resource-claim messages to investigate an interface-up stall. Expected: successful build; no credentials in added diagnostics.

Base commit `8be851bc884ec0a31646b28242a54fcf827bb987` plus the uncommitted port change identified in the [source/image manifest](logs/2026-10-30_INT-33-manifest.json). Automated builder: Codex. Profile: `SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 TEST=1`. BUILD PASS; `git diff --check` passed. [Build output](logs/2026-10-30_INT-33-build.txt). Flashing and physical Wi-Fi/MQTT verification remain pending. The previously connected image revision/checksum is unknown; its USB output stopped after interface-up began while IR readings continued, including after the operator reported a USB power cycle. This observation does not establish a driver root cause.

## INT-34 — Diagnostic image startup observation — 2026-10-30

[Case and image identity](logs/2026-10-30_INT-34-case.json) defined the capture. Operator reported BOOTSEL ready after USB-only instructions; Codex copied the INT-33 TEST image and captured USB output. Base commit `8be851bc884ec0a31646b28242a54fcf827bb987` plus the uncommitted diagnostic change identified by the INT-33 manifest; flashed UF2 SHA-256 `d4d9a9a0694401e83f185df83b6ecac100d5bc872cce906ce5b0aeeff38e4a31`. This is working-tree diagnostic evidence, not a committed-revision qualification. Wiring was not inspected, calibration was not performed, TEST inhibits motors.

Observed: interface initialization advanced, PIO allocation returned 4, DMA allocations returned 0 and 1, then network join was attempted. [Initial capture](logs/2026-10-30_INT-34-console.txt) includes link states 1, 2 and -2 (SSID not found); [retry capture](logs/2026-10-30_INT-34-retries.txt) includes repeated states 1 and 2. Neither capture contains link-up/IP acquisition or MQTT ready. No robot connection appeared in the bounded broker checks. Wireless reception remains FAIL/unresolved. The trace changes timing and the previous image identity is unknown, so this does not establish a fix for the earlier initialization stall. Awaiting operator confirmation of configured network availability/2.4 GHz access.

INT-34 continuation: operator confirmed hotspot on and laptop connected. macOS reported channel 6 (2 GHz, 20 MHz), WPA2 Personal and laptop IP `172.20.10.3`; SSID was redacted, so an exact name comparison was unavailable. Credentials were checked only for presence/edge whitespace, without printing them. [Subsequent USB retry capture](logs/2026-10-30_INT-34-network-confirmed-retry.txt) is retained separately. No robot broker connection observed in subsequent bounded checks.

INT-34 authentication finding: the subsequent capture also includes link state -3 (authentication rejected). Operator-supplied SSID matched the local configured SSID exactly; no SSID or password is copied into this record. The laptop network security was reported WPA2 Personal. This makes credential verification the next step; it does not prove the password is wrong or exclude a radio/authentication compatibility problem. Operator asked to compare the local password with hotspot settings without sharing the password.


## Stationary wider recovery — INT-35

Case defined 2026-10-30 before accepted run. Host-only synthetic IR and encoder
inputs, no wiring/power/calibration. Verify recovery emits STOP until encoder
stationarity, commands opposite wheel directions for pivots, holds action IDs
and relative targets during each leg, reverses at +/-100 degree endpoints
(including encoder wrap), and stops after four legs or excessive heading.
Verify visible-line braking, bounded alignment at a wide found heading,
distinct stationary confirmations, timeout, hazard and failed-action stop.
Run the full pure suite and wireless normal firmware build; record source
hashes plus exact base commit. These checks cannot establish physical tracking.

Result 2026-10-30, tester Codex: PASS full host suite and wireless normal
build (`SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0
TEST=0`). Base commit `8be851bc884ec0a31646b28242a54fcf827bb987` plus
working-tree sources identified by the [manifest](logs/2026-10-30_INT-35-manifest.json).
[Accepted host output](logs/2026-10-30_INT-35-final-host.txt),
[accepted build](logs/2026-10-30_INT-35-accepted-build.txt).
The earlier Docker attempt was denied socket access; a later build caught a
kernel/libc `size_t` conflict from adding math.h to the diagnostic wrapper.
The wrapper now computes absolute excursion without that header; all attempt
outputs are preserved separately. Final incremental build has no warnings;
text/data/BSS 379200/396/84136 bytes. The image includes existing uncommitted
Terrain, servo and Wi-Fi diagnostic changes. No firmware flashed and no
physical reacquisition pass claimed. `git diff --check` passes.

## Dashboard next-junction direction — INT-36

Case defined 2026-10-30 before acceptance run. Host-only inputs, no wiring,
power or physical calibration. Verify all four test directions remain pending
until a confirmed junction, override barcode reads, consume once, reject
replacement/non-following/junction-encounter requests and clear on local stop.
Verify binary type-7 valid actions, invalid/reserved bits, short frames,
single-slot acknowledgement ordering and consecutive duplicate rejection.
Run full pure suite, dashboard HTTP checks and wireless normal firmware build.
Record exact base commit and working-tree manifest. A host/build pass cannot
establish physical turn accuracy or a flashed robot connection.

Result 2026-10-30, tester Codex: PASS full host suite, dashboard JS/HTTP checks
and wireless normal firmware build (`SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip
MQTT=1 CRASH_REPORT=0 TEST=0`). Base commit `8be851bc884ec0a31646b28242a54fcf827bb987` plus
working-tree source hashes in the [manifest](logs/2026-10-30_INT-36-manifest.json).
[Host output](logs/2026-10-30_INT-36-host.txt),
[dashboard script](logs/2026-10-30_INT-36-dashboard.txt),
[HTTP checks](logs/2026-10-30_INT-36-http-retry.txt),
[firmware build](logs/2026-10-30_INT-36-build-retry.txt).
Initial Docker and local HTTP attempts were blocked by sandbox socket access;
the retry checks passed in Docker. All attempt logs are retained.
Local dashboard rebuilt/restarted; [build log](logs/2026-10-30_INT-36-dashboard-build.txt).
Image text/data/BSS: 379520/396/84140 bytes. Existing unrelated working-tree
Terrain, obstacle, recovery and Wi-Fi changes are included in this image.
No firmware flashed, no live robot direction submitted, no physical pass claimed.

## Junction speed-controller reversal — INT-37

Case defined 2026-10-30 before execution. User reports wheels repeatedly jerking
or reversing at a junction. Host-only synthetic monotonic encoder counts, no
physical calibration. Reproduce opposite motor duty while a discrete action
still requests wheel travel in its original direction; verify forward/reverse
distance and both turn directions brake rather than reverse on speed overshoot.
Preserve legitimate opposite wheel directions in pivots and positional
overshoot correction. Run full host suite and wireless normal build, with exact
base commit/source manifest. No physical encoder fault or turn success inferred.

Result 2026-10-30, tester Codex: reproduced in host controller with monotonic
encoder counts. Reverse-distance target -28 mm emitted positive duty +0.016708
after 16 counts of reverse travel over 20 ms, before reaching its target; see
[pre-fix reproduction](logs/2026-10-30_INT-37-reproduction.txt). This establishes
a software reversal mechanism, not the cause of the user's physical vibration.

After the direction-aware speed clamp: PASS full pure suite, with both distance
signs, both turn signs and preserved positional overshoot correction;
[final host output](logs/2026-10-30_INT-37-host-final.txt). Wireless normal build
PASS (`SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 TEST=0`);
[build output](logs/2026-10-30_INT-37-build-retry.txt). First build request did
not start because automatic permission review timed out; permitted retry passed.
Base commit `8be851bc884ec0a31646b28242a54fcf827bb987` plus [source manifest](logs/2026-10-30_INT-37-manifest.json).
No encoder polarity, geometry or PID gains changed. Existing unrelated local
changes are present in the image. No firmware flashed or motor command sent;
physical vibration and encoder health still require a mounted retest.

## Turn endpoint reactivation — INT-38

Case defined 2026-10-30 before execution. User confirms INT-37 firmware flashed
and reports sweep/junction turn jitter, onset uncertain. Host-only encoder
fixtures: one wheel enters turn tolerance before its partner, then drifts in
either direction. Expect that wheel to remain braked for the action. Both
wheels must currently be within tolerance for DONE; drift cannot be hidden
by a prior arrival. Persistent drift must time out FAILED. New action resets
arrival state. Test both turn signs, full host suite and wireless build.
This does not establish the cause of physical mid-turn jitter.

Result 2026-10-30, tester Codex: reproduced endpoint reactivation in the host
controller after the INT-37 change. An arrived turn wheel drifted 12 counts
and was driven again at duty +0.340960 in the synthetic case;
[pre-fix output](logs/2026-10-30_INT-38-reproduction.txt).
With per-wheel turn-arrival latches: PASS full host suite, both turn signs,
drift either side, current-position completion check, timeout failure and
new-action reset; [host output](logs/2026-10-30_INT-38-host.txt).
Wireless normal build PASS (`SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1
CRASH_REPORT=0 TEST=0`); [build output](logs/2026-10-30_INT-38-build.txt).
Base commit `8be851bc884ec0a31646b28242a54fcf827bb987` plus [manifest](logs/2026-10-30_INT-38-manifest.json).
No gains, encoder signs or tolerances changed. No firmware flashed or robot
command sent in this case. Physical endpoint jitter needs retesting; the cause
of user-reported possible mid-turn jitter remains unconfirmed.

## Sensor dashboard ledger — INT-39

Case defined 2026-10-30 before validation. Host/browser only, no robot commands,
firmware build or physical sensor pass. Extend the existing dashboard with
labelled telemetry measurements and units. Verify synthetic valid-zero and
missing results, encoder-derived speed, barcode, completed obstacle/hump
results, stale messaging, and unchanged junction-control gating. Inspect the
render at desktop and narrow widths using a separate loopback fixture server;
fixtures are illustrative, not observations. Record base HEAD and source hashes
with results. No raw IR/IMU/servo readings may be implied by decoded telemetry.

## Remove fixed junction advance — INT-39

Case defined 2026-10-30 before execution. Nicholas requests removing the 25 mm
advance entirely. Host fixtures cover immediate straight FOLLOW over broad
black, no pre-turn DISTANCE action or DONE dependency, both queued/manual
turns and straight/unmapped encounters, thin-patch preservation, distinct
black acquisition qualification, repeated/wrapped counts, fresh exit sensing,
stop/invalid input/obstacle/deadline handling. Preserve post-turn clearance.
Run full pure suite and wireless normal build; record exact base/source
manifest. No physical junction centring or jitter resolution inferred.

## Sensor dashboard ledger — INT-40

The sensor-ledger case initially reserved INT-39 above while another active
chat independently reserved INT-39 for junction behaviour. INT-39 belongs to
that junction case; this dashboard case is now INT-40. Its procedure remains
as defined in the sensor-ledger entry above. Existing junction evidence is
untouched; dashboard captures and manifest have distinct INT-40 paths.

Result 2026-10-30, INT-40, tester Codex: PASS dashboard script regression
and sensor-value checks ([host output](logs/2026-10-30_INT-40-host.txt)),
[HTTP regression](logs/2026-10-30_INT-40-http.txt), and
[decoder self-test](logs/2026-10-30_INT-40-decoder.txt). Synthetic browser
checks covered live, empty, invalid and stale readings at 1280/390 px;
no horizontal overflow at either width. Captures and isolated fixture server
are in [evidence](logs/2026-10-30_INT-40/). Rebuilt/restarted only the dashboard
container; observed the new layout receiving the robot's existing telemetry
at port 8080. This observation is not sensor calibration or a physical pass.
Base HEAD `8be851bc884ec0a31646b28242a54fcf827bb987`, uncommitted source hashes
in [manifest](logs/2026-10-30_INT-40-manifest.json). Existing unrelated changes
remain. No robot command sent or firmware built/flashed for this case.
Impeccable launcher/detector could not execute (permission denied); manual
visual review used existing DESIGN.md and the craft floor instead.

Result 2026-10-30, tester Codex: PASS full pure suite and wireless normal
firmware build (`SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1
CRASH_REPORT=0 TEST=0`). [Host output](logs/2026-10-30_INT-39-host.txt),
[build output](logs/2026-10-30_INT-39-build.txt),
[source/image manifest](logs/2026-10-30_INT-39-manifest.json).
Base commit `8be851bc884ec0a31646b28242a54fcf827bb987` plus manifest working-tree hashes.
Removed the fixed-distance crossing configuration, CROSS phase and distance
completion handler. Broad-black encounters use straight FOLLOW and retain
qualification/exit semantics and guard deadlines. Existing turn commands,
U-turn pause and separate 60 mm post-turn clearance remain. Image text/data/BSS
379224/396/84140 bytes, including unrelated existing working-tree changes.
No firmware flashed and no physical jitter/turn-centre pass claimed.

## One-screen dashboard — INT-41

Case defined 2026-10-31 before execution. Host/browser only. Redesign the
existing offline dashboard for the user's 1440 × 900 desktop viewport: all
five numbered subsystems, seven reported Mission states, sensor measurements,
faults, custom four-letter mapping, chart and events visible together. Verify
live/empty/stale/fault fixtures, mapping validation and acknowledgement,
existing junction gating, and compact desktop layout. Synthetic fixtures must
be visibly labelled; do not send robot commands. Save new evidence and exact
base HEAD plus working-tree source hashes. No firmware or physical pass.

## Toggleable binary steering buildup — INT-42

Case defined 2026-10-31 before execution. Run the pure host suite with the
existing default controller and the Mission suite with `LINE_BINARY_RAMP=1`.
Check sign-only input, gentle start, monotonic bounded buildup, reversal
reset with slew limiting, slowdown, duplicate acquisitions, gap restart,
zero error, timestamp wrap, stop/reset and existing Mission safety/junction
regressions. Preserve default PID float-reference checks. Record base HEAD
and source hashes because the working tree contains uncommitted changes.
No firmware execution, physical steering or junction discrimination pass is
implied. Binary settings remain provisional D07.

Result 2026-10-31, INT-41, tester Codex: PASS host dashboard tests
([output](logs/2026-10-31_INT-41/host.txt)) and HTTP request validation
([output](logs/2026-10-31_INT-41/http.txt)). Seven Mission state markers,
duplicate/offline gates and mapping ACK arriving before HTTP sequence response
are covered. No robot command sent. Browser synthetic live, empty, stale,
invalid and all-owner fault fixtures fit at 1440 × 900 without document
scrolling. Custom mapping A/B/C/D accepted through the synthetic UI; repeated
A rejected. 1366 × 768 scrolls vertically (830px content); 800px narrow window
has no horizontal overflow. See [browser measurements](logs/2026-10-31_INT-41/browser-checks.json),
[accepted mapping](logs/2026-10-31_INT-41/mapping-1440.png),
[fault view](logs/2026-10-31_INT-41/fault-1440.png) and
[fixture source](logs/2026-10-31_INT-41/fixture-preview.py).
Rebuilt/restarted only the local dashboard container
([build log](logs/2026-10-31_INT-41/dashboard-build.txt)); port 8080 serves
the redesign, showing actual robot offline / no readings at observation time
([capture](logs/2026-10-31_INT-41/dashboard-8080.png)).
Exact base revision and uncommitted source hashes are recorded in the
[manifest](logs/2026-10-31_INT-41/manifest.json). This is host/browser evidence,
not firmware execution or a physical pass. Existing unrelated edits preserved.
Impeccable context launcher could not execute (permission denied); project
context and craft floor were read directly and rendering reviewed manually.

Result 2026-10-31, INT-42, tester Codex: PASS host pure suite including both
controller modes ([host output](logs/2026-10-31_INT-42-host.txt)); PASS default
wireless normal firmware build ([build output](logs/2026-10-31_INT-42-build.txt));
PASS binary-mode Mission ARM compilation with the same build flags
([ARM output](logs/2026-10-31_INT-42-binary-arm.txt)). Source: base HEAD
`9bcab80f4ae299694eff7b2564600883fee0891d` plus uncommitted files identified
by the [source manifest](logs/2026-10-31_INT-42-manifest.json). Default remains
`LINE_BINARY_RAMP=0`. No firmware flashed or executed; no physical straight,
tight-curve, recovery-alignment or junction pass.

## Subsystem health table revision — INT-43

Case defined 2026-10-31 before execution. Host/browser only. Verify five rows
in S1–S5 order, Mission state/faults retained above the table, Motion direction
in Reading and left/right speed in Detail. User clarified calibration details
belong to S3/S4, not S5. Current wire frame has no calibration booleans: test
positive usable-line/valid-hump evidence, reported faults and unknown cases
without claiming absence of faults proves calibration. Preserve S5 dimensions,
sensor values, mapping and junction behaviour. Save exact source identity and
new evidence; no firmware flash or robot commands.

Result 2026-10-31, INT-43, tester Codex: PASS dashboard host regression
([output](logs/2026-10-31_INT-43/host.txt)). Browser fixtures confirm ordered
S1–S5 rows, Forward and separate L/R speed, S3/S4 Calibrated and Fault / not
calibrated labels, and Mission line-lost fault in the top strip. Saved
[synthetic preview](logs/2026-10-31_INT-43/final-preview.png),
[fault view](logs/2026-10-31_INT-43/fault.png) and
[browser readings](logs/2026-10-31_INT-43/browser-checks.json).
The final clean preview rendered at 1280 × 720 with 817px content height;
the browser viewport override did not consistently apply, so this run does
not claim a fresh 1440 × 900 layout pass. Rebuilt only the dashboard container
([build output](logs/2026-10-31_INT-43/dashboard-build.txt)); port 8080 verified
with five rows, actual robot offline. No robot commands, firmware build or
physical calibration pass. Exact base revision and uncommitted source hashes
are in the [manifest](logs/2026-10-31_INT-43/manifest.json).

## Repository audit — INT-44

Case defined 2026-10-31 before verification in
[case definition](logs/2026-10-31_INT-44/case.json). Audit current working-tree
source, run pure/dashboard host checks, build all four README profiles, and
distinguish software findings from pending physical acceptance. No robot
commands, firmware flashing or production-code changes.

Result 2026-10-31, INT-44, tester Codex: PASS existing pure host suite,
dashboard JavaScript and loopback HTTP checks; PASS all four firmware profile
builds in an isolated temporary container with placeholder credentials and
network disabled. Initial HTTP sandbox bind failure is preserved alongside
the successful authorized retry. Four P2 findings: encoder-mean discontinuity,
Terrain displacement lost on pending acquisition, CLI success for rejected
mapping ACKs, and dashboard tests omitted from CI. Encoder/CLI defects have
saved executable reproductions; Terrain/CI findings are source-path reviews.
See [audit and evidence](logs/2026-10-31_INT-44/audit.md).
Source: base HEAD `9bcab80f4ae299694eff7b2564600883fee0891d` plus uncommitted
files identified by the [initial manifest](logs/2026-10-31_INT-44/manifest.json)
and [full source manifest](logs/2026-10-31_INT-44/source-manifest-full.json).
No firmware execution or physical pass is claimed. Existing edits preserved.

## Repository audit fixes — INT-45

Case defined before execution in [case definition](logs/2026-10-31_INT-45/case.json).
Resolve the four INT-44 findings; include the existing audited working tree as
explicitly authorized by the user. No physical test or firmware flash.

Result 2026-10-31, INT-45, tester Codex: PASS on exact source commit
`2005c687629a6dd8cfdc986ea45cc332f18e0855`. Pure host suite covers divergent wheel lifetime counts,
per-wheel wrap/reverse/pivot travel, junction qualification at the half-range
boundary, pending Terrain travel and failure/discard behavior
([host output](logs/2026-10-31_INT-45/host-final.txt)). PASS dashboard JavaScript
([output](logs/2026-10-31_INT-45/dashboard.txt)), loopback HTTP validation
([output](logs/2026-10-31_INT-45/http.txt)) and CLI OK/rejected/unknown ACK,
sequence matching and snapshot regression ([output](logs/2026-10-31_INT-45/cli.txt)).
CI now explicitly runs all three laptop/dashboard tests. All four README
firmware profiles compile/link/convert successfully in an isolated container
with placeholder credentials and network disabled
([build output](logs/2026-10-31_INT-45/firmware-builds.txt)). Common flags:
`SMP=1 CONSOLE=usb_cdc WIFI=cyw43 CRASH_REPORT=0`; offline
`NET=none MQTT=0`, wireless `NET=lwip MQTT=1`, each TEST=1 and TEST=0.
Only logged build warnings are the existing upstream lwIP unused function.
[Manifest](logs/2026-10-31_INT-45/manifest.json) source hashes were verified
against the committed files. No firmware execution, physical calibration,
stop timing or floor acceptance is claimed.

INT-45 continuation, 2026-10-31, tester Codex: PR #139 at exact revision
`ec86fcf18f9fe0ff0f15d8ee6b2fbbe680ca9c6b` received a failed GitHub check
before any job steps started: account payments/spending limit blocked Actions
([annotation](logs/2026-10-31_INT-45/github-ci-blocked.json)). This is an
environment failure, not a test failure. The local Linux C checks passed; the
combined script then stopped because the existing `robot-build` image lacks
Python ([output](logs/2026-10-31_INT-45/linux-host-and-size.txt)). The remaining
Linux decoder check passed in the existing Python dashboard image
([output](logs/2026-10-31_INT-45/linux-python.txt)), and the ARM size probe passed
separately ([output](logs/2026-10-31_INT-45/arm-size.txt)). MotionCommand and
MotionHealth remain 12 bytes on that target compiler.

The user subsequently requested disabling CI. Removed automatic push and
pull-request triggers; manual workflow_dispatch retains the regression/build
commands for explicit runs. README describes this change. No software changes
affecting the passed host/firmware checks were made after the tested source
commit; the final workflow/source identity is in
[final manifest](logs/2026-10-31_INT-45/final-source-manifest.json).

## Repository audit — INT-46

Case defined before targeted probes in [case definition](logs/2026-10-31_INT-46/case.json).
Result 2026-10-31, tester Codex, exact source revision
`5e63fe0e90776eb834fd3d27ba438a0f8e017d8e`: existing pure host, dashboard
JavaScript, CLI and loopback HTTP suites PASS. Initial HTTP sandbox binding
restriction was resolved by an authorized retry. Three P2 findings reproduced:
acceleration-dominated data accepted as a valid hump, queued direction RX
surviving connection loss, and malformed laptop telemetry accepted/displayed.
The offline bridge delivery is reproduced; downstream Mission acceptance is a
source-path finding. No production changes or new firmware/physical pass.
See [audit, scope and limits](logs/2026-10-31_INT-46/audit.md) and
[source manifest](logs/2026-10-31_INT-46/manifest.json).

## Repository audit fixes — INT-47

Defined before verification in [case definition](logs/2026-10-31_INT-47/case.json).
Result 2026-10-31, tester Codex: PASS on exact source commit
`54e5ec44a52dde0c17c9fe43b21469d36e905f1e`.
Terrain rejects acceleration-dominated and unsteady/differential/reverse wheel
travel, requires the existing provisional 200 ms steady window and retains
encounter disqualification until completion. Completed valid maxima persist.
RX connection-loss discard preserves producer/consumer index ownership.
Both codecs reject invalid telemetry sentinels/actions; Reporting clears invalid
validity flags, and the laptop rejects reserved telemetry/event bits before
updating display or speed history.

Pure host suite PASS ([committed-source output](logs/2026-10-31_INT-47/host-committed.txt));
dashboard JavaScript, CLI and loopback HTTP PASS
([dashboard](logs/2026-10-31_INT-47/dashboard.txt),
[CLI](logs/2026-10-31_INT-47/cli.txt), [HTTP](logs/2026-10-31_INT-47/http.txt)).
All four README profiles compile/link/convert successfully
([committed-source builds](logs/2026-10-31_INT-47/firmware-committed.txt)), using
`SMP=1 CONSOLE=usb_cdc WIFI=cyw43 CRASH_REPORT=0`, offline
`NET=none MQTT=0` and wireless `NET=lwip MQTT=1`, each TEST=0 and TEST=1.
An isolated source copy used placeholder credentials with container networking
disabled; concurrent barcode changes were excluded. ARM probes show
TerrainContext remains 32 bytes; TerrainTravelQuality is 12 bytes.
Only firmware warnings are the existing upstream lwIP unused function.
[Final source identity](logs/2026-10-31_INT-47/final-source-manifest.json).

Initial host verification failed because Reporting's old clamp fixture
produced validity/action values now rejected by the stricter codec
([initial output](logs/2026-10-31_INT-47/host-initial.txt)); both the clamp and
fixture were corrected. Initial firmware compilation failed because the new
public evidence header included newlib math types after kernel types
([initial build](logs/2026-10-31_INT-47/firmware-initial.txt)); moving the
helper into the existing kernel-free algorithm source fixed that boundary.
Successful working-source checks are also retained; the committed-source
checks above establish the final source results.

No robot commands, flashing, firmware execution, physical calibration, timing
or floor trials. Quality thresholds remain provisional D06/D07/D10 settings;
software suppression does not establish a measured height-error bound.

## Terrain header build compatibility — INT-48

Case defined before verification on 2026-10-31: eliminate the newlib
`<math.h>` / micro T-Kernel `size_t` collision introduced by the Terrain
travel-quality helper in the public evidence header. Run the existing pure
host suite and compile/link/convert all four README firmware profiles:
`SMP=1 CONSOLE=usb_cdc WIFI=cyw43 CRASH_REPORT=0`, offline
`NET=none MQTT=0` and wireless `NET=lwip MQTT=1`, each with TEST=0 and TEST=1.
Use isolated source snapshots, placeholder credentials and no container network.
No physical sensing, calibration or firmware-execution acceptance is claimed.

The first snapshot moves the unchanged helper into `terrain_algo.c` and passes
all checks. Concurrent work subsequently removes the Terrain audit edits from
the checkout, including the helper and public math include. The final source
snapshot therefore verifies that current state separately; it does not restore
those removed edits. Preserve the earlier logs and manifest as intermediate
evidence; `final-*` artifacts identify the final verification.

| Date | ID | Source | Tester | Result | Evidence / limits |
| --- | --- | --- | --- | --- | --- |
| 2026-10-31 | INT-48 | Base `5e63fe0e90776eb834fd3d27ba438a0f8e017d8e` plus [final working-source manifest and UF2 checksums](logs/2026-10-31_INT-48/final-manifest.json) | Codex | PASS: host suite and all four firmware profiles | [Host](logs/2026-10-31_INT-48/final-host.txt), offline [TEST=0](logs/2026-10-31_INT-48/final-firmware-none-test0.txt) / [TEST=1](logs/2026-10-31_INT-48/final-firmware-none-test1.txt), wireless [TEST=0](logs/2026-10-31_INT-48/final-firmware-lwip-test0.txt) / [TEST=1](logs/2026-10-31_INT-48/final-firmware-lwip-test1.txt). Arm GCC 13.2.1; compile/link/UF2 conversion only, placeholder credentials, no board execution. |

## Barcode branch integration — INT-49

Defined before verification on 2026-10-31: integrate the barcode enable-time
quiet-zone reference and flipped-print regression cases with main's Terrain
quality, stale-RX discard and strict telemetry codec fixes. Preserve the
S3-U20 / INT-48 evidence and INT-46 / INT-47 evidence; resolve only their
appended integration-log conflict. Run the complete pure host suite, dashboard
JavaScript, CLI and loopback HTTP checks, plus all four README firmware
profiles in an isolated source copy with placeholder credentials and disabled
container networking. Expected: barcode tests and audit regressions pass;
all four images compile/link/convert; no prior evidence is replaced.
No robot commands, firmware execution, calibration or physical pass.

Result 2026-10-31, INT-49, tester Codex: PASS on exact integrated source
`545a6a381b589f033076bd04df825469e2f0b537`. Full pure host suite
([output](logs/2026-10-31_INT-49/host.txt)), dashboard JavaScript, CLI and
loopback HTTP regressions passed
([dashboard](logs/2026-10-31_INT-49/dashboard.txt),
[CLI](logs/2026-10-31_INT-49/cli.txt), [HTTP](logs/2026-10-31_INT-49/http.txt)).
All four README firmware profiles compiled, linked and converted to UF2
([build output](logs/2026-10-31_INT-49/firmware.txt)), with
`SMP=1 CONSOLE=usb_cdc WIFI=cyw43 CRASH_REPORT=0`; offline
`NET=none MQTT=0` and wireless `NET=lwip MQTT=1`, each TEST=0 and TEST=1.
Only warnings are the existing upstream lwIP unused function. The build used
a git archive of that exact source, placeholder credentials and no container
network. [Manifest and image checksums](logs/2026-10-31_INT-49/manifest.json).
Main's S1/S4/decoder sources are unchanged by this integration. The historical
S3-U20 failed full-build attempt and both audit records are preserved; the
combined build above passes with the Terrain header fix now on main.
No flashing, firmware execution, GPIO timing or physical barcode pass.

## Consolidated completion tracking — INT-50

Defined in [case](logs/2026-10-31_INT-50/case.json) before verification.
Result 2026-10-31, tester Codex, exact documentation/source revision
`30ec622432bc061418e2769aeb8f0359e1507109`: PASS for eight changed
Markdown guides, 244 local links/anchors and inbound plan links, with firmware
source unchanged from main `565bae7`
([checks](logs/2026-10-31_INT-50/documentation-checks.txt)). Original test
records and evidence are unchanged; append-only snapshots preserve the issue
and PR115 before consolidation.

Issue #117 now owns the live complete plan and remaining acceptance.
Superseded #129–#133 and obsolete PR115 were closed with transfer comments;
branches were preserved ([closure record](logs/2026-10-31_INT-50/github-closures.json)).
The [appended all-stage audit](logs/2026-10-31_INT-50-completion.json) and
[source manifest](logs/2026-10-31_INT-50/source-manifest.json) refresh the
completion check: **exit 1 / BLOCKED**, not stale exit 2 or COMPLETE
([output](logs/2026-10-31_INT-50/completion-check.txt)). This verifies
record/source consistency, not robot operation. Existing INT-49 host/build
evidence is reused for unchanged firmware; no new build, flash, firmware
execution, physical calibration or timing pass was performed.

## Retire the duplicate plan — INT-51

Defined before verification in [case](logs/2026-10-31_INT-51/case.json).
Result 2026-10-31, tester Codex, exact source commit
`fc9fa5d1af192433854d0440071e2954e32ee7d4`: PASS for 16 active Markdown
guides, 336 local links/anchors and updated issue anchors, with `plan.md`
removed and all 44 prior pending items mapped into ten issue117 checklist
groups ([scope map](logs/2026-10-31_INT-51/scope-map.json),
[documentation checks](logs/2026-10-31_INT-51/documentation-checks.txt)).
An isolated git archive excluded a concurrent unrelated README edit.
Firmware and historical test records/evidence were preserved. Old `plan.md`
references within recorded historical logs retain their original revision
meaning; the retired document remains available in Git history at
[the consolidation baseline](https://github.com/shauniee/Autonomous-Robotic-Car/blob/b398126ba7465463d38099c42ff0e3a4dcee1046/plan.md).

Issue117 remains OPEN; its shorter checklist links the owning requirements
and physical procedures. The [appended stage audit](logs/2026-10-31_INT-51-completion.json)
and [source manifest](logs/2026-10-31_INT-51/source-manifest.json) return
**BLOCKED / exit 1** ([output](logs/2026-10-31_INT-51/completion-check.txt)).
Existing INT-49 software checks apply to unchanged firmware; no new firmware
builds, hardware execution, calibration or physical acceptance are claimed.

## Recovery brake audit — INT-52

Case defined before execution in [case](logs/2026-10-31_INT-52/case.json).
2026-10-31, tester Codex, base commit
`fc9fa5d1af192433854d0440071e2954e32ee7d4`, with exact tested sources in
[manifest](logs/2026-10-31_INT-52/source-manifest.json).
Result: **FAIL recovery handoff acceptance; bug reproduced on host**.
The [reproduction](logs/2026-10-31_INT-52/repro.c) feeds Mission's sweep TURN
and line-detection braking STOP through the actual Motion controller.
STOP interrupts a running turn and returns RD_MOTION_FAILED; subsystem2's
failure guard latches STOP, and Mission's motion_stopped guard enters STOPPED.
The reproduction models that wrapper latch as a health input; it does not
execute the kernel task. [Output](logs/2026-10-31_INT-52/repro-output.txt)
records the observed controller statuses. The existing full pure suite passes
([output](logs/2026-10-31_INT-52/existing-host-suite.txt)); existing recovery
tests check Mission braking commands without feeding those commands back
through Motion. No firmware source changed, firmware built/flashed or
physical test performed. This identifies a software defect, not the exact
cause of an unrecorded physical run.

## Obstacle history — INT-53

Case defined before execution in [case](logs/2026-10-31_INT-53/case.json).
2026-10-31, tester Codex, base commit `8981d04620d5537e4f5b68980528b452b752abb1`,
with exact tested working-tree sources in
[manifest](logs/2026-10-31_INT-53/source-manifest.json).
Result: **PASS host and synthetic UI checks**.
[Dashboard regression](logs/2026-10-31_INT-53/dashboard.txt) checks completion-only
entries, duplicate delivery, equal-sized obstacles, missing width, numbering and
the 200-entry bound. [Decoder self-test](logs/2026-10-31_INT-53/decoder.txt)
checks structured obstacle event fields.
[HTTP checks](logs/2026-10-31_INT-53/http-retry.txt) pass after retry outside
the socket-restricted sandbox; the initial loopback bind was denied.
[Visual checks](logs/2026-10-31_INT-53/visual-checks.json) cover live and empty
1440 × 900 layouts and narrow overflow;
[desktop capture](logs/2026-10-31_INT-53/desktop.png) uses synthetic measurements.
The [fixture server](logs/2026-10-31_INT-53/fixture-preview.py) is loopback-only.
Rebuilt/restarted the local dashboard
([build](logs/2026-10-31_INT-53/dashboard-build.txt)); port 8080 serves the exact
updated HTML. History records completion events observed during this page
session; refresh clears it. Width comes from the latest observed non-SCAN
telemetry, while length comes from the completion event. Missing width stays
explicit. No firmware build, flash, robot measurement or physical pass claimed.

## Recovery brake handoff fix — INT-54

2026-10-31, tester Codex, base commit `8981d04620d5537e4f5b68980528b452b752abb1`,
with exact working-tree sources in [manifest](logs/2026-10-31_INT-54/source-manifest.json).
[Case](logs/2026-10-31_INT-54/case.json) was defined before execution.
Result: **PASS host regression suite** ([output](logs/2026-10-31_INT-54/host.txt)).
Explicit STOP interrupts active TURN/DISTANCE with zero duties and IDLE;
a subsequent action runs. Missing/expired active commands still return FAILED.
The recovery test now feeds the line-detection brake through real Motion control
and returns its status to Mission; alignment completes to FOLLOW_LINE.
Default/guarded Motion and PID/binary Mission profiles pass. The wrapper's
failure-latch condition is modeled as Mission input, not executed as a kernel task.
No firmware build/flash or physical run; this does not establish on-target recovery.

## Stable visible recovery handoff — INT-55

2026-10-31, tester Codex, base commit `8981d04620d5537e4f5b68980528b452b752abb1`,
exact working-tree sources and normal wireless UF2 hash in
[manifest](logs/2026-10-31_INT-55/source-manifest.json).
[Case](logs/2026-10-31_INT-55/case.json) defined before execution.
Initial compilation failed because the bypass still uses the alignment-error
constant ([initial output](logs/2026-10-31_INT-55/host.txt)); retained that
constant for bypass and reran the suite. **PASS host suite**
([final output](logs/2026-10-31_INT-55/host-final.txt)), including PID/binary
Mission and default/guarded Motion. Off-center visibility now brakes through
actual Motion to IDLE and returns to FOLLOW_LINE after distinct stationary
samples. Cached/moving samples cannot confirm, loss resets confirmation,
and existing sweep/hazard/expiry/timeout checks pass. Motion's wrapper latch
is modeled as feedback, not run as a kernel task.
**PASS normal wireless build**, TEST=0, command and size in manifest; no flash
or firmware execution. The >1 s loss qualification and 150 ms encoder quiet
interval remain software settings; no observed physical timing is claimed.

## NAVIGATE jitter audit — INT-56

2026-10-31, tester Codex, base commit `8981d04620d5537e4f5b68980528b452b752abb1`;
exact working-tree sources/command in [manifest](logs/2026-10-31_INT-56/source-manifest.json).
[Case](logs/2026-10-31_INT-56/case.json) defined before execution.
**PASS host audit reproduction assertions**, not physical acceptance
([source](logs/2026-10-31_INT-56/repro.c), [output](logs/2026-10-31_INT-56/output.txt)).
A qualified junction with no pending barcode/manual action selects straight
NAVIGATE/CONFIRM, not TURN or the post-turn clearance action. With fresh LOST
IR it commands FOLLOW at 90 mm/s, zero steering, action 1 for the reproduced
2 s interval; it does not use ordinary line-loss recovery. Three distinct
VISIBLE acquisitions complete confirmation without requiring a barcode.
Finding: confirmation can drive blindly until the 8 s navigation timeout;
visible samples before confirmation also get zero steering despite nonzero
line error. This can miss the outgoing line; it does not prove motor jitter.
Separately, scripted real-position drift of an arrived clearance wheel
produces zero/reverse/forward/reverse duty while its partner is behind. This
DISTANCE behavior lacks TURN's arrival hold, but it is not on the unmapped
straight route and does not explain that route by itself.
Source audit found stable action identity in both paths, not per-release
action restarts. Quadrature transition/wrap/sign code was inspected; no
physical encoder waveform, high-rate counts/duty trace or flashed-image
identity was available. The live dashboard was already STOPPED/IDLE with LOST
IR when inspected; that snapshot cannot establish preceding transitions.
No firmware source or tuning changed, no build/flash or powered test in this audit.

## Line tracking lifecycle audit — INT-57

2026-10-31, tester Codex, base commit
`8981d04620d5537e4f5b68980528b452b752abb1`, exact working-tree inputs in
[manifest](logs/2026-10-31_INT-57/source-manifest.json).
[Case](logs/2026-10-31_INT-57/case.json) defined before execution.
Existing full pure suite **PASS**; new PID/binary audit assertions **PASS**
([suite](logs/2026-10-31_INT-57/existing-host-suite.txt),
[PID](logs/2026-10-31_INT-57/pid.txt),
[binary](logs/2026-10-31_INT-57/binary.txt)). These assertions reproduce
behavior, not physical acceptance. A synthetic stationary visible detection
at 80° search-relative heading and error +100 resumes FOLLOW at 431 mm/s,
steering +8/127. Immediate LOST retains that command through 1000 ms before
recovery restarts. Separately, default PID retains a 0.40 integral across
800 ms of LOST; subsequent error -64 initially produces +68/127 steering.
Binary mode clears history and produces -8/127. Current explicit recovery
STOP returns Motion IDLE. Full lifecycle, source locations, prior-build
comparison, adjacent NAVIGATE issue and proposed correction direction are in
[audit](logs/2026-10-31_INT-57/audit.md).
No application source/tuning changed, firmware built/flashed or physical
test performed. The Motion wrapper failure latch is modeled as Mission input;
kernel scheduling and physical line geometry are not simulated.

## Wider repeating recovery sweep — INT-58

2026-10-31, tester Codex, base commit
`8981d04620d5537e4f5b68980528b452b752abb1`; uncommitted source identity in
[before manifest](logs/2026-10-31_INT-58/before-manifest.json) and
[final manifest](logs/2026-10-31_INT-58/source-manifest.json).
[Case](logs/2026-10-31_INT-58/case.json) defined before execution.
Initial regression **FAIL as expected** against the original controller:
after reaching the endpoint and braking, synthetic coast to target +10 degrees
trips the old target +5 degree hard bound before reversal
([output](logs/2026-10-31_INT-58/host-before.txt)). This demonstrates one way
to stop after a sweep; it does not identify the cause of the user's physical run.

Separated endpoint tolerance from the hard overshoot bound. Trial targets are
now +120/-120/+120/-120 degrees, maximum 240 mm/s per pivot wheel; the hard
heading bound is +/-140 degrees. Four legs, 30 s, excursion, sensing, obstacle,
stop and stationarity constraints remain. Stable-visible detection still
brakes and confirms before tracking. Values are proposals owned by D03/D07/D08.

Full host suite **PASS** ([output](logs/2026-10-31_INT-58/host-after.txt)),
including PID/binary Mission, default/guarded Motion, endpoint coast/reversal,
and four Mission-issued sweep legs through actual pure Motion control with
distinct action IDs and STOP-to-IDLE feedback. Kernel wrapper latch behavior
is modeled; no kernel task or physical movement is executed.
Normal wireless firmware **BUILD PASS**, TEST=0, with command, exit code and
UF2 SHA-256 in [build result](logs/2026-10-31_INT-58/build-result.json) and
[console](logs/2026-10-31_INT-58/build.txt). The first sandbox Docker access
was denied; the build succeeded with authorized Docker socket access.
No flash or physical angle/speed/stopping-distance pass claimed.

## Straight NAVIGATE slowdown and jitter audit — INT-59

2026-10-31, tester Codex; user clarified the symptom is crossing straight
without a turn. Base commit `8981d04620d5537e4f5b68980528b452b752abb1` plus
exact working-tree [manifest](logs/2026-10-31_INT-59/source-manifest.json).
[Case](logs/2026-10-31_INT-59/case.json) defined before execution.
**PASS behavior assertions and existing pure suite**, not physical acceptance
([reproduction](logs/2026-10-31_INT-59/repro-output.txt),
[suite](logs/2026-10-31_INT-59/host.txt)).
Straight crossing enters NAVIGATE/CONFIRM with a default STOP command;
confirmation requests FOLLOW at 90 mm/s, zero steering, action 1, including
2 s of fresh LOST and off-center visible samples before confirmation completes.
The third visible sample enters FOLLOW_LINE with another default STOP;
the following release requests 450 mm/s for zero error. A repeated encounter
repeats both brake boundaries; actual Motion consumption of a 10 ms pulse
depends on scheduling. TURN and clearance DISTANCE are absent from this route.
The existing idealized quantized wheel plant at steady 90 mm/s has zero
braked/reverse samples in its final 2 s; it is not a friction/inertia model.
Current source and UF2 match INT-58; this specific navigation problem remains
unchanged ([comparison](logs/2026-10-31_INT-59/build-comparison.json)).
Full findings, source locations, correction scope and limitations:
[audit](logs/2026-10-31_INT-59/audit.md).
No application changes, new build, flash, live physical trace or running-image
verification. Slow command and boundary STOPs are reproduced; steady-state
physical jitter remains unproven by these host tests.

## Navigation trigger and low-speed transition check — INT-60

2026-10-31, tester Codex; base commit
`8981d04620d5537e4f5b68980528b452b752abb1` and exact working-tree inputs in
[manifest](logs/2026-10-31_INT-60/source-manifest.json).
[Case](logs/2026-10-31_INT-60/case.json) defined before execution.
**PASS synthetic transition assertions**, not physical acceptance
([source](logs/2026-10-31_INT-60/transition.c),
[output](logs/2026-10-31_INT-60/transition-output.txt)).
Actual Motion controller with the existing idealized quantized plant requests
zero duty twice during the first 2 s of a 450 -> 90 mm/s transition, then has
no zero/reverse duty samples in the following 4 s. With an intervening STOP,
the 90 mm/s segment has no zero/reverse samples in this model. Final duty is
approximately 0.066..0.098. This does not model physical stiction or inertia
and does not confirm continuous low-speed motor jitter.

Source inspection: NAVIGATE begins on exit from qualified BROAD_BLACK, not
on barcode reception alone. Qualification requires a distinct usable broad
acquisition with mean forward travel >=6 encoder counts since broad entry.
No held turn selects straight CONFIRM; left/right/U-turn selects navigation
turn phases, then clearance and confirmation. Three distinct usable VISIBLE
acquisitions complete confirmation; absent confirmation, the whole navigation
deadline is 8 s. Broad-black evidence cannot establish a real junction alone;
a curve or patch satisfying the same software criteria can trigger this path.

Bounded read-only local dashboard SSE capture succeeded after retry with
network access ([events](logs/2026-10-31_INT-60/live-events-retry.txt)).
Observed STOPPED/IDLE with LOST, fixed wheel counts; no navigation run was
captured. The curl timeout is the deliberate six-second observation bound,
not a robot fault. Initial sandbox connection failure is preserved separately.
No commands sent, motor movement requested, application source changed,
firmware built/flashed or physical jitter pass claimed. Low-speed friction
remains a hypothesis requiring a moving wheel-count/duty trace.

## Full-speed 25 mm straight junction exit — INT-61

2026-10-31, tester Codex, base commit
`8981d04620d5537e4f5b68980528b452b752abb1` plus exact working-tree sources in
[manifest](logs/2026-10-31_INT-61/source-manifest.json).
[Case](logs/2026-10-31_INT-61/case.json) defined before execution.
**PASS full pure host suite** ([output](logs/2026-10-31_INT-61/host.txt)),
including PID/binary Mission and default/guarded Motion.

On qualified broad-black exit with no queued left/right/U-turn, including a
queued straight instruction, Mission now enters STRAIGHT and immediately
commands FOLLOW at normal line-follow speed with zero steering. After >=25 mm
of mean signed encoder travel from that exit pose, it runs ordinary tracking
in the same release. No distinct-visible confirmation, position-action
deceleration or default STOP boundary remains on this straight path. Ordinary
hump reduction still applies. Queued-turn clearance/confirmation is retained.

Tests cover visible readings not completing early, completion with LOST,
immediate visible-error tracking on completion, encoder wrap, threshold boundary,
asymmetric/reverse travel, one-time straight instruction consumption, hump
speed, local stop, front hazard, invalid/stale IR, Motion stop and timeout.
Straight phase also requires fresh functional IR in the Mission adapter;
VISIBLE classification is not required for segment completion. No new stored
or wire fields; existing origin storage and three-bit phase field are reused.

Normal wireless TEST=0 firmware **BUILD PASS**
([console](logs/2026-10-31_INT-61/build.txt),
[command/UF2 hash](logs/2026-10-31_INT-61/build-result.json)).
No flash or physical run. Encoder scale, update interval and mechanical slip
limit actual distance accuracy; 25 mm is a requested software travel threshold,
not measured displacement. Physical jitter resolution remains unverified.

## Immediate simple 70 degree recovery — INT-62

2026-10-31, tester Codex; base commit
`8981d04620d5537e4f5b68980528b452b752abb1` and exact working-tree sources in
[manifest](logs/2026-10-31_INT-62/source-manifest.json).
[Case](logs/2026-10-31_INT-62/case.json) defined before execution.
**PASS full pure host suite** ([output](logs/2026-10-31_INT-62/host.txt)),
including PID/binary Mission, default/guarded Motion and disabled recovery policy.

The first fresh usable LOST during ordinary tracking commands STOP and enters
REACQUIRE immediately. Held/manual navigation is cleared, barcode acquisition
stopped, scan canceled and side-watch disabled. Once stationary, recovery
repeats +/-70 degree pivots with no four-leg cutoff. Endpoint braking and
stationarity before reversal remain. Either usable VISIBLE side immediately
brakes the pivot, enters FOLLOW_LINE and restarts barcode capture, whose start
API discards stale events. There is no stationary visible-confirmation streak.
Invalid/stale or broad-black sensing does not authorize tracking handoff.
Existing 30 s, excursion, overshoot, hazard, stop and actuator guards remain.
The deliberate junction-turn and 25 mm straight segment retain their own
policies; if still LOST when the straight segment ends, ordinary tracking now
immediately enters this recovery rather than holding the old correction.

Host checks cover first-loss cancellation and release gating, timestamp wrap,
left/right visible handoff through pure Motion STOP-to-IDLE, broad-black
rejection, eight alternating legs with coast and distinct action IDs, timeout,
overshoot, hazard, cancel, functional/stale sensing and actuator failure.
Removed obsolete loss-delay timestamps/pending flag and recovery confirmation
storage; the old leg counter becomes a one-bit direction. No wire change.
Barcode driver/capture shutdown and restart are source-inspected API calls,
not executed hardware evidence. Kernel stop-latch behavior is modeled as input.

Normal wireless TEST=0 firmware **BUILD PASS**
([console](logs/2026-10-31_INT-62/build.txt),
[command/UF2 SHA-256](logs/2026-10-31_INT-62/build-result.json)).
No flash or physical run; angle accuracy, stopping response and the retained
240 mm/s trial pivot speed remain uncalibrated. Immediate loss/handoff has no
noise-confirmation interval, so physical IR noise may cause repeated mode changes.

## 500 ms recovery takeover qualification — INT-63

2026-10-31, tester Codex; base commit
`8981d04620d5537e4f5b68980528b452b752abb1` plus exact working-tree inputs in
[manifest](logs/2026-10-31_INT-63/source-manifest.json).
[Case](logs/2026-10-31_INT-63/case.json) defined before execution.
**PASS full pure host suite** ([output](logs/2026-10-31_INT-63/host.txt)),
including PID/binary Mission, default/guarded Motion and disabled recovery.

Fresh usable white/white now retains the last tracking steering and its speed
for <500 ms. At >=500 ms of continuous acquisition evidence, Mission brakes,
cancels pending navigation/barcode/scan/side-watch and enters the existing
simplified +/-70 degree recovery. VISIBLE or BROAD_BLACK resets qualification.
Acquisition gaps beyond the freshness budget restart qualification; cached
samples do not advance it and stale/invalid inputs still stop. Stop, obstacle,
Motion health and recovery bounds remain. The deliberate 25 mm straight segment
remains separate; ordinary qualification begins if LOST at segment completion.
Restored private first/latest LOST timestamps and one pending bit, reset on
state changes; no wire change or blocking wait.

Tests cover retained speed/steering and held manual direction during grace,
499 ms FOLLOW versus exactly 500 ms STOP/REACQUIRE, cancellation at takeover,
visible/broad reset, zero/wrapped timestamps, missing acquisition intervals,
cached/stale readings and immediate stop/invalid-input handling. Existing eight-leg
sweep, either-side handoff and bounds tests pass. Recovery policy itself is
unchanged from INT-62 after takeover; firmware-level hardware/queue operations
are source-inspected rather than executed by host tests.

Normal wireless TEST=0 firmware **BUILD PASS**
([console](logs/2026-10-31_INT-63/build.txt),
[command/UF2 SHA-256](logs/2026-10-31_INT-63/build-result.json)).
No flash or physical run. At nominal 450 mm/s, the requested grace interval
permits 225 mm of commanded travel; actual displacement and safe recovery
timing remain unmeasured. This changes takeover timing, not tracking gains.

## Short physical sweep angle investigation — INT-64

2026-10-31, tester Codex; user reports recovery at a curve appears to sweep
about 10 degrees repeatedly until the 30 s budget expires. No measured angle,
encoder trace or final recovery console line supplied. Exact working-tree
inputs/base commit in [manifest](logs/2026-10-31_INT-64/source-manifest.json);
[case](logs/2026-10-31_INT-64/case.json) defined before execution.
**PASS estimated-angle assertions**, not physical acceptance
([source](logs/2026-10-31_INT-64/angle.c),
[output](logs/2026-10-31_INT-64/output.txt)).
With configured 360 decoded counts/revolution, 65 mm wheel diameter and
130 mm track, opposite travel of 20 counts per wheel estimates 10 degrees
and does not reverse recovery; opposite travel of 130 counts estimates
65 degrees and triggers endpoint braking, followed by a -135 relative target
toward the opposite -70 endpoint. Requested 70 degrees corresponds to 140
counts per pivot wheel; the 5-degree arrival tolerance explains only 5 degrees,
not the reported 60-degree discrepancy. Local UF2 still matches INT-63;
flashed identity is not verified.

Repeated small physical reversals can indicate incorrect encoder/geometry
scale or slip; this is a hypothesis, not an established cause. No ten-degree
endpoint is configured. Early VISIBLE detection instead exits to FOLLOW_LINE;
its relevance depends on the actual observed state transitions. Requested
one-full-wheel-revolution count measurement while GP21 is held to check the
unconfirmed D03 scale. No geometry, firmware source, tuning or image changed;
no powered movement, new build or flash performed.

## User-reported encoder revolution count — INT-65

2026-10-31, observation reported by Nicholas: before 31142, after 33655 in
response to the one-full-wheel-revolution check with GP21 held. Difference is
2513 decoded counts; wheel identity, other-wheel measurement and repeatability
are pending. This is user-reported evidence, not a Codex-executed physical test
or calibrated firmware pass. [Observation](logs/2026-10-31_INT-65/observation.json)
and [current source manifest](logs/2026-10-31_INT-65/source-manifest.json), base
`8981d04620d5537e4f5b68980528b452b752abb1`.

The observed scale is 6.981 times the current configured 360 decoded counts
per revolution. Assuming current diameter/track and no slip, an estimated
70 degree pivot corresponds to 10.03 physical degrees, consistent with the
reported short sweep. Endpoint tolerance alone does not explain this ratio.
A global change to 2513 without reconciling speed settings multiplies target
counts/s by the same factor; current feedforward/P terms would saturate initial
follow and recovery requests at the duty cap. Review speeds before powered use.
No application/geometry/tuning change, new build, flash or movement requested.

## Operator-assisted encoder calibration — INT-66

2026-10-31, tester Codex with Nicholas as physical operator; case defined before
the new observation in [case](logs/2026-10-31_INT-66/case.json).
Exact source/base identity in
[manifest](logs/2026-10-31_INT-66/source-manifest.json).
Initial read-only dashboard capture contains five READY/IDLE frames at fixed
left=65527, right=1, reporting zero derived wheel speed
([raw](logs/2026-10-31_INT-66/left-before-events.txt),
[summary](logs/2026-10-31_INT-66/left-before-summary.json)).
The sixteenth-bit wrapping count is a baseline, not a calibration result.
Requested one manual forward revolution of the marked left wheel with GP21
held and wheels raised, without resetting the board. Awaiting operator reply
and end-count observation. The curl timeout deliberately bounds observation;
it is not a robot failure. No powered motion commanded or firmware changed.
Right-wheel 2513-count user-reported observation is retained as a separate
source; left scale, repeatability and measured-distance trial remain pending.

INT-66 baseline retry: Nicholas reported wheel movement during setup, so the
initial left=65527 baseline is unsuitable for measuring a revolution.
A new three-second read-only capture contains six READY/IDLE frames at fixed
left=65501, right=0, reporting zero derived wheel speed
([raw](logs/2026-10-31_INT-66/left-before-retry1-events.txt),
[summary](logs/2026-10-31_INT-66/left-before-retry1-summary.json)).
Use left=65501 for the requested manual revolution. Original evidence remains;
no end-count/result yet and no firmware changes or powered motion commanded.

INT-66 left-revolution result: Nicholas replied done after the requested manual
forward revolution. Six READY/IDLE telemetry frames then held left=2454,
right=0, with zero derived wheel speed
([raw](logs/2026-10-31_INT-66/left-after-events.txt),
[summary](logs/2026-10-31_INT-66/left-after-summary.json)).
Wrap-safe signed travel from 65501 is +2489 counts. The right wheel's separately
user-reported +2513 differs by 24 counts, 0.96% of their mean. Manual revolution
placement and repeatability are unmeasured; these observations support an
approximately 2500-count scale, not a rated or final calibrated constant.
No code/tuning/image changed. Requested setup for a manual 500 mm straight roll
with GP21 held; capture a fresh baseline only after floor placement, before
the operator pushes. No powered calibration movement is authorized by this step.

INT-66 manual-distance baseline: after Nicholas reported the 500 mm path
prepared and the robot positioned, five READY/IDLE frames held left=2210,
right=9, reporting zero derived wheel speeds
([raw](logs/2026-10-31_INT-66/distance-before-events.txt),
[summary](logs/2026-10-31_INT-66/distance-before-summary.json)).
Requested a straight manual 500 mm forward roll with GP21 held, both wheels
rolling without sliding/lifting and the same body reference point at the
finish. Awaiting operator completion and end-count capture. No motor commands,
firmware changes or calibration result yet for this distance trial.

INT-66 manual-distance result: Nicholas replied done. Six READY/IDLE frames
held left=8043, right=5825, reporting zero derived wheel speed
([raw](logs/2026-10-31_INT-66/distance-after-events.txt),
[summary](logs/2026-10-31_INT-66/distance-after-summary.json)).
Relative to the positioned baseline, deltas are +5833 and +5816 counts over
the operator-reported 500 mm, giving left 11.666 and right 11.632 counts/mm,
mean 11.649. Agreement is 0.292% of the mean. Distance placement, repeatability
and uncertainty were not independently measured; no powered motor run occurred.
INT-67 records applying this first rolling calibration in software.

## Measured rolling-scale application — INT-67

2026-10-31, tester Codex; base commit
`8981d04620d5537e4f5b68980528b452b752abb1` plus exact working-tree sources in
[manifest](logs/2026-10-31_INT-67/source-manifest.json).
[Case](logs/2026-10-31_INT-67/case.json) defined before implementation/checks.
Applied INT-66's mean 11.649 counts/mm directly in configuration, replacing the
provisional 360-count/65-mm odometry formula. Laptop decoder reads the same
constant. The two manual revolution counts remain observed evidence rather
than inferred motor ratings or wheel diameters. One common scale is used;
separate per-wheel correction is not inferred from a single 0.292%-different roll.

Mission speeds are reconciled to approximately retain prior counts/s:
follow 68, broad 14, turn/clear 15, recovery 36, bypass 9 and bypass-line
5 mm/s. Wheel PID gains, duty cap and 16-bit telemetry counter layout remain.
The bounded action/navigation deadline becomes 12 s to accommodate real
180-degree turns and clearance at reconciled speeds. The 500 ms recovery delay,
25 mm straight segment, +/-70 sweep and 30 s recovery deadline remain.
Individual 2000 mm action and 1200 mm recovery travel fit signed half range.
Whole-bypass heading now uses modular differential displacement, avoiding
false headings when common forward wheel travel crosses a signed half range.

**PASS full pure host suite**
([verified output](logs/2026-10-31_INT-67/host-verified.txt)), including
default/guarded Motion, PID/binary Mission, calibrated measurement replay,
retained count-speed bands, long-travel differential heading, decoder and
Terrain regression. Initial runs exposed scale-dependent Terrain fixtures
([initial](logs/2026-10-31_INT-67/host-initial.txt),
[second](logs/2026-10-31_INT-67/host-final.txt),
[third](logs/2026-10-31_INT-67/host-complete.txt)); fixtures now express physical
travel/velocity and feed identical rounded encoder counts to both current and
legacy-reference estimators. TURN drift fixtures scale excursions beyond the
angle tolerance, and overspeed fixtures retain the corresponding low count
target after unit correction. No Terrain production algorithm changed.

Wireless normal TEST=0 and offline motor-inhibited TEST=1 **BUILD PASS**, no
warnings ([normal](logs/2026-10-31_INT-67/wireless-normal-build.txt),
[TEST](logs/2026-10-31_INT-67/offline-test-build.txt),
[commands/normal UF2 hash](logs/2026-10-31_INT-67/build-results.json)).
Local dashboard was rebuilt/restarted to apply the same decoder scale
([verification](logs/2026-10-31_INT-67/dashboard-verification.txt),
[result](logs/2026-10-31_INT-67/dashboard-result.json)).
No firmware flashed or powered motion commanded. Actual angle depends on the
still-provisional track width and slip; distance repeatability, achieved speed,
wheel PID calibration and physical jitter/stop acceptance remain pending.

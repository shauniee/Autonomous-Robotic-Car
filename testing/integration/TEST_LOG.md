# Integration test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)
- [Audits, not tests](#audits-not-tests)

One row per run. Write-ups from before 2026-11-03, and build and host output, are at git tag `test-logs-full-2026-11-03`: `git show test-logs-full-2026-11-03:testing/integration/TEST_LOG.md`. `+` after a commit means it also had uncommitted changes. `INT-39b`, `INT-139b`, `INT-140b` and `INT-141b` are second records that reused an ID.

## Test cases

| ID | Scope | Method | Expected |
|---|---|---|---|
| INT-03 | line_barcode request / Motion stop and expiry / health | Host actual Motion service with fake kernel, normal and TEST profiles | Stop independent of mutex; expired commands latch; reset cannot replay; TEST inhibits outputs |
| INT-04 | Reproducible template build and ownership | Normal and TEST=1 SMP builds; upstream byte comparison and static audit | Both UF2s; unchanged runtime/build source; init-before-start; native ADC disabled; no app network calls |
| INT-05 | Real line_barcode task and unchanged Motion API contract | Host task_ir with fake GPIO/ADC/Motion, normal and TEST profiles | Recovery remains enabled; acquisition time preserved; GP21/reset/start, failed init/acquisition/health/publish/reset, inhibition and missed release fail closed |
| INT-06 | Read-only MQTT service | Actual mqtt_utk.c with fake lwIP/kernel/clock; 25 reconnect cycles | QoS0/nonretained; 500 ms stream; 3 s refusal/immediate-failure/alloc retry; skip unavailable/oversized/failed publication; latest reconnect; no timer accumulation |
| INT-07 | Telemetry isolation from line_barcode and Motion | Actual line_barcode loop with fake hardware/Motion/kernel, MQTT enabled, normal and TEST; snapshot available or always busy | GP21 dominates; reset/start/recovery/watchdog and timestamps unchanged; failed telemetry never authorizes motion |
| INT-08 | Mac broker and terminal transport | Historical retired host runner, isolated loopback broker and CLI subscriber, actual JSON encoder fixture | Valid bounded JSON arrives; refusal/restart/reconnect; new subscriber gets no retained history; no Pico claim |
| INT-09 | Wireless/offline build and preservation | Normal/TEST MQTT and offline normal builds, source/Word/ownership audit | All UF2s compile; Motion/control policy, kernel and historical sources preserved; credentials excluded |

| INT-10 | Restored Comms kernel reporting transport | Wireless TEST Pico run with broker/operator fault injection; pending | Status subscription before Online; 1 s heartbeat, retained online/offline will, authenticated broker; latest line_barcode schema unchanged; malformed/drive/start/stop/calibration requests rejected; status response rate bounded; 15 s attempt deadline and 1..30 s retry; link loss clears fragments, no stale replay or control calls; measure timer/stack budget |

Later cases are named in their result rows.

## Results

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
|---|---|---|---|---|---|---|
| 2026-10-08 | INT-03 | logs/2026-10-08_source-sha256.json | Codex | PASS | at tag | Fake kernel, not scheduler/physical proof |
| 2026-10-08 | INT-03/05 | logs/2026-10-08_s3-proportional-source-sha256.json | Codex host run | PASS | host log | Real line_barcode loop with fake inputs/Motion and unchanged real Motion service with fake kernel; both profiles |
| 2026-10-08 | INT-03/05/06/07 | [source manifest](logs/2026-10-08_s3-mqtt-source-sha256.json) | Codex host run | PASS | [host log](logs/2026-10-08_s3-mqtt-host-checks.txt) | Fake kernel/hardware/network; unchanged Motion actual service also rerun in both profiles |
| 2026-10-08 | INT-04 | Same manifest; base HEAD in JSON | Codex | PASS | logs/2026-10-08_migration-evidence.json; at tag | Both profiles compiled; no firmware flashing |
| 2026-10-08 | INT-04 | Same manifest; base HEAD in JSON | Codex | PASS | [audit](logs/2026-10-08_s3-proportional-evidence.json); normal build; TEST build | Both UF2s; 564 protected files unchanged; no firmware flashing |
| 2026-10-08 | INT-08 | Initial pre-PTY host harness | Codex Mac run | FAIL | [initial harness log](logs/2026-10-08_s3-mqtt-broker-initial.txt) | Timed out waiting for pipe-buffered CLI debug output; fixed harness with terminal-style PTY; robot code unaffected |
| 2026-10-08 | INT-08 | Same manifest as final host run | Codex Mac run | PASS | [broker log](logs/2026-10-08_s3-mqtt-broker.txt) | Real Mosquitto loopback, synthetic C-encoded fixture; not firmware/Pico transport |
| 2026-10-08 | INT-09 | Same manifest and base HEAD | Codex | PASS | [audit](logs/2026-10-08_s3-mqtt-evidence.json); normal MQTT build; TEST MQTT build; offline build | Built without flashing; private config content excluded; unrelated line_barcode formatting/comment edits left uncommitted |
| 2026-10-08 | INT-09 | `57f3b95`+ | Codex static audit | PASS | [post-sync audit](logs/2026-10-08_s3-mqtt-main-sync.json) | Integrated concurrent documentation update; 160 local links valid; runtime unchanged. Later user driver formatting is token-equivalent to the tested source; no new hardware claim |
| 2026-10-09 | INT-09 | [restoration manifest](logs/2026-10-09_s1-restore_evidence.json) | Codex build/static audit | PASS | wireless TEST, wireless normal, offline TEST, offline normal, strict compile | Comms reporting/service restored; Motion–Obstacle, app startup/shared header, kernel and Make source unchanged; example credentials only; no flashing |
| 2026-10-09 | INT-10 | Same manifest | On-target run pending | PENDING | No Pico service log | Build and laptop-fixture checks do not execute kernel transport/retry/subscription on hardware |
| 2026-10-29 | INT-12 | `fe39e3a` | Codex | PASS (host, build) | [INT-12_stage1-manifest.json](logs/2026-10-29_INT-12_stage1-manifest.json) | Restructure stage #130: stop, expiry and reset policy |
| 2026-10-29 | INT-13 | `b862817` | Codex | PASS (host, build) | [INT-13_stage2-manifest.json](logs/2026-10-29_INT-13_stage2-manifest.json) | Stage #131: seven states, action IDs, recovery |
| 2026-10-29 | INT-14 | `ab54479` | Codex | PASS (host, build) | [INT-14_stage3-manifest.json](logs/2026-10-29_INT-14_stage3-manifest.json) | Stage #132: mapping exchanges, event retry, Terrain faults |
| 2026-10-29 | INT-15 | `407cbfa` | Codex | PASS (host, static, build) | [INT-15_stage4-manifest.json](logs/2026-10-29_INT-15_stage4-manifest.json) | Stage #133: tasks, mutexes, motor writer, no app networking |
| 2026-10-29 | INT-16 | `0448890`+ | Codex | PASS (host, static, build) | [INT-16-manifest.json](logs/2026-10-29_INT-16-manifest.json) | Production gates, 150 mm trigger, synthetic profile values |
| 2026-10-29 | INT-17 | `0448890`+ | Codex | BLOCKED | [INT-17_completion.json](logs/2026-10-29_INT-17_completion.json) | Full-plan audit; hardware unavailable |
| 2026-10-29 | INT-18 | `0448890`+ | Codex | Retired | At tag | Container tooling, removed in `73c8a93`. Tooling checks passed; wireless TEST build failed on undefined `BYPASS_REACQUIRE` |
| 2026-10-29 | INT-19 | `0448890`+ | Codex | FAIL, then PASS (host, build) | [INT-19-ci-block.txt](logs/2026-10-29_INT-19-ci-block.txt) | PR integration gate. First host compile failed on removed bypass fields; passed after integration fixes |
| 2026-10-30 | INT-21 | `62ee522`+ | Codex | PASS (host, build) | [INT-21-manifest.json](logs/2026-10-30_INT-21-manifest.json) | Scan ID regression and cleanup |
| 2026-10-30 | INT-23 | `62ee522`+ | Codex | PASS (host) | [INT-23-manifest.json](logs/2026-10-30_INT-23-manifest.json) | Rear-edge geometry |
| 2026-10-30 | INT-24 | `62ee522`+ | Codex | PASS (host, build, static); plan BLOCKED | [INT-24-accepted-abi.txt](logs/2026-10-30_INT-24-accepted-abi.txt), [INT-24-accepted-resources.json](logs/2026-10-30_INT-24-accepted-resources.json) | Resource and binary-protocol audit. Also SYS-12 |
| 2026-10-30 | INT-25 | `62ee522` | Codex | PASS (host, build, static); plan BLOCKED | [INT-25-accepted-static.txt](logs/2026-10-30_INT-25-accepted-static.txt), [INT-25-accepted-abi.txt](logs/2026-10-30_INT-25-accepted-abi.txt) | Event contention and unused-path cleanup |
| 2026-10-30 | INT-26 | `cc3edb3`+ | Codex | PASS (build) | At tag | Recovery stop diagnostics. Hardware diagnosis pending |
| 2026-10-30 | INT-27 | `cc3edb3` | Codex | PASS (host, build) | [INT-27-manifest.json](logs/2026-10-30_INT-27-manifest.json) | Recovery stall tuning. Physical recovery pending |
| 2026-10-30 | INT-28 | `cc3edb3` | Codex | PASS (host, build) | [INT-28-manifest.json](logs/2026-10-30_INT-28-manifest.json) | Sustained line-loss qualification. Physical run pending |
| 2026-10-30 | INT-29 | `cc3edb3` | Codex | PASS (build) | [INT-29-manifest.json](logs/2026-10-30_INT-29-manifest.json) | Recovery failure inputs. Physical failure attribution pending |
| 2026-10-30 | INT-30 | `cc3edb3` | Codex | Bug reproduced, then PASS (host, build) | [INT-30-manifest.json](logs/2026-10-30_INT-30-manifest.json) | Forward FOLLOW slowdown. Physical run pending |
| 2026-10-30 | INT-31 | `cc3edb3` | Codex | PASS (host, build) | [INT-31-manifest.json](logs/2026-10-30_INT-31-manifest.json) | Recovery wheel-speed range. Physical sweep pending |
| 2026-10-30 | INT-32 | `cc3edb3` | Nicholas | PASS (host, build) | [INT-32-manifest.json](logs/2026-10-30_INT-32-manifest.json) | Stall configuration integration. Nicholas confirmed the default-disabled stall guard |
| 2026-10-30 | INT-33 | `8be851b`+ | Codex | PASS (build) | [INT-33-manifest.json](logs/2026-10-30_INT-33-manifest.json) | Wi-Fi startup diagnostic build. Motor-inhibited TEST image |
| 2026-10-30 | INT-34 | `8be851b`+ | Codex | OBSERVED | [INT-34-case.json](logs/2026-10-30_INT-34-case.json), [INT-34-console.txt](logs/2026-10-30_INT-34-console.txt) | Diagnostic image startup observation. Wi-Fi join reached link state -3, authentication rejected; cause not proven |
| 2026-10-30 | INT-35 | `8be851b`+ | Codex | PASS (host, build) | [INT-35-manifest.json](logs/2026-10-30_INT-35-manifest.json) | Stationary wider recovery. No physical reacquisition |
| 2026-10-30 | INT-36 | `8be851b`+ | Codex | PASS (host, dashboard) | [INT-36-manifest.json](logs/2026-10-30_INT-36-manifest.json) | Dashboard next-junction direction. No physical turn check |
| 2026-10-30 | INT-37 | `8be851b` | Codex | Bug reproduced, then PASS (host, build) | [INT-37-reproduction.txt](logs/2026-10-30_INT-37-reproduction.txt) | Junction speed-controller reversal. Software reversal found; not shown to be the physical vibration |
| 2026-10-30 | INT-38 | `8be851b` | Codex | Bug reproduced, then PASS (host, build) | [INT-38-reproduction.txt](logs/2026-10-30_INT-38-reproduction.txt) | Turn endpoint reactivation. Cause of physical mid-turn jitter not established |
| 2026-10-30 | INT-39 | Not recorded | Not recorded | Case only | — | Sensor dashboard ledger. Results recorded under INT-40 |
| 2026-10-30 | INT-39b | In manifest | Nicholas | PASS (host, build) | — | Remove fixed junction advance. Result recorded under INT-40; no physical junction check |
| 2026-10-30 | INT-40 | `8be851b` | Codex | PASS (host, dashboard, build) | [2026-10-30_INT-40/](logs/2026-10-30_INT-40/) | Sensor dashboard ledger. Dashboard seen at port 8080; no physical sensor check |
| 2026-10-31 | INT-41 | Not recorded | Not recorded | PASS (host, dashboard) | — | One-screen dashboard |
| 2026-10-31 | INT-42 | `9bcab80`+ | Codex | PASS (host, build) | [2026-10-31_INT-41/](logs/2026-10-31_INT-41/) | Toggleable binary steering buildup. No physical steering check |
| 2026-10-31 | INT-43 | In manifest | Codex | PASS (host, dashboard) | [2026-10-31_INT-43/](logs/2026-10-31_INT-43/) | Subsystem health table revision |
| 2026-10-31 | INT-48 | `5e63fe0` | Codex | PASS (host, build) | [2026-10-31_INT-48/](logs/2026-10-31_INT-48/) | Terrain header build compatibility |
| 2026-10-31 | INT-49 | `545a6a3`+ | Codex | PASS (host, dashboard, build) | [2026-10-31_INT-49/](logs/2026-10-31_INT-49/) | Barcode branch integration. At `545a6a3`; no physical barcode check |
| 2026-10-31 | INT-52 | `fc9fa5d` | Codex | FAIL (host) | [2026-10-31_INT-52/](logs/2026-10-31_INT-52/) | Recovery brake audit. Recovery handoff bug reproduced on host |
| 2026-10-31 | INT-53 | `8981d04`+ | Codex | PASS (host, dashboard) | [2026-10-31_INT-53/](logs/2026-10-31_INT-53/) | Obstacle history |
| 2026-10-31 | INT-54 | `8981d04`+ | Codex | PASS (host) | [2026-10-31_INT-54/](logs/2026-10-31_INT-54/) | Recovery brake handoff fix |
| 2026-10-31 | INT-55 | `8981d04`+ | Codex | PASS (host, build) | [2026-10-31_INT-55/](logs/2026-10-31_INT-55/) | Stable visible recovery handoff |
| 2026-10-31 | INT-56 | `8981d04`+ | Codex | PASS (host) | [2026-10-31_INT-56/](logs/2026-10-31_INT-56/) | NAVIGATE jitter audit. Audit assertions; physical jitter not proven |
| 2026-10-31 | INT-57 | `8981d04`+ | Codex | PASS (host) | [2026-10-31_INT-57/](logs/2026-10-31_INT-57/) | Line tracking lifecycle audit. Audit assertions; no source change |
| 2026-10-31 | INT-58 | `8981d04`+ | Codex | Failed as expected, then PASS (host) | [2026-10-31_INT-58/](logs/2026-10-31_INT-58/) | Wider repeating recovery sweep |
| 2026-10-31 | INT-59 | `8981d04`+ | Codex | PASS (host) | [2026-10-31_INT-59/](logs/2026-10-31_INT-59/) | Straight NAVIGATE slowdown and jitter audit. Audit assertions; no source change |
| 2026-10-31 | INT-60 | `8981d04`+ | Codex | PASS (host) | [2026-10-31_INT-60/](logs/2026-10-31_INT-60/) | Navigation trigger and low-speed transition check. Synthetic transitions only |
| 2026-10-31 | INT-61 | `8981d04`+ | Codex | PASS (host, build) | [2026-10-31_INT-61/](logs/2026-10-31_INT-61/) | Full-speed 25 mm straight junction exit |
| 2026-10-31 | INT-62 | `8981d04`+ | Codex | PASS (host, build) | [2026-10-31_INT-62/](logs/2026-10-31_INT-62/) | Immediate simple 70 degree recovery |
| 2026-10-31 | INT-63 | `8981d04`+ | Codex | PASS (host, build) | [2026-10-31_INT-63/](logs/2026-10-31_INT-63/) | 500 ms recovery takeover qualification |
| 2026-10-31 | INT-64 | In manifest | Codex | PASS (host) | [2026-10-31_INT-64/](logs/2026-10-31_INT-64/) | Short physical sweep angle investigation. Estimated angle only |
| 2026-10-31 | INT-65 | `8981d04` | Nicholas, Codex | OBSERVED | [2026-10-31_INT-65/](logs/2026-10-31_INT-65/) | User-reported encoder revolution count. User-reported: 2513 counts for one wheel revolution, 6.981 times the configured 360 |
| 2026-10-31 | INT-66 | In manifest | Nicholas, Codex | MEASURED | [2026-10-31_INT-66/](logs/2026-10-31_INT-66/) | Operator-assisted encoder calibration. Manual 500 mm roll: left 11.666, right 11.632, mean 11.649 counts/mm; distance not independently measured |
| 2026-10-31 | INT-67 | `8981d04`+ | Codex | PASS (host, build) | [2026-10-31_INT-67/](logs/2026-10-31_INT-67/) | Measured rolling-scale application. Applies INT-66 |
| 2026-10-31 | INT-69 | `4365e10`+ | Codex | PASS (host) | [2026-10-31_INT-69/](logs/2026-10-31_INT-69/) | Digital IR tracking and half-speed navigation |
| 2026-10-31 | INT-70 | `4365e10`+ | Codex | PASS (host) | [2026-10-31_INT-70/](logs/2026-10-31_INT-70/) | Remove digital IR software calibration |
| 2026-10-31 | INT-71 | `4365e10`+ | Codex | PASS (host) | [2026-10-31_INT-71/](logs/2026-10-31_INT-71/) | Correct digital IR polarity |
| 2026-10-31 | INT-72 | `4365e10`+ | Codex | PASS (host) | [2026-10-31_INT-72/](logs/2026-10-31_INT-72/) | Stronger digital steering and junction speed ramp |
| 2026-10-31 | INT-73 | `4365e10`+ | Codex | PASS (host) | [2026-10-31_INT-73/](logs/2026-10-31_INT-73/) | Reverse digital tracking correction |
| 2026-10-31 | INT-74 | `4365e10`+ | Codex | PASS (host) | [2026-10-31_INT-74/](logs/2026-10-31_INT-74/) | Diagnose GP20 start while Mission displays READY. Firmware diagnosis pending |
| 2026-10-31 | INT-75 | In manifest | Codex | PASS (host) | [2026-10-31_INT-75/](logs/2026-10-31_INT-75/) | Verify digital IR through Mission and wheel outputs |
| 2026-10-31 | INT-76 | In manifest | Codex | PASS (host, build) | [2026-10-31_INT-76/](logs/2026-10-31_INT-76/) | Execute queued direction on first fresh both-black sample |
| 2026-10-31 | INT-77 | In manifest | Codex | PASS (host, build) | [2026-10-31_INT-77/](logs/2026-10-31_INT-77/) | Reverse mounted IR steering correction. User reported the physical steering direction |
| 2026-10-31 | INT-78 | In manifest | Codex | PASS (host) | [2026-10-31_INT-78/](logs/2026-10-31_INT-78/) | Single-wheel 1.5x IR correction |
| 2026-10-31 | INT-79 | `86ce2d8`+ | Codex | PASS (host) | [2026-10-31_INT-79/](logs/2026-10-31_INT-79/) | Reported IMU calibration. Not a physical calibration |
| 2026-10-31 | INT-80 | `86ce2d8`+ | Not recorded | PASS (host, build) | [2026-10-31_INT-80/](logs/2026-10-31_INT-80/) | Smooth tracking with strong curve correction |
| 2026-10-31 | INT-81 | `86ce2d8`+ | Not recorded | PASS (host, build) | [2026-10-31_INT-81/](logs/2026-10-31_INT-81/) | Stronger sustained curves and modest speed increase |
| 2026-10-31 | INT-82 | `86ce2d8`+ | Not recorded | PASS (host, build) | [2026-10-31_INT-82/](logs/2026-10-31_INT-82/) | Half-to-full curve speed ramp |
| 2026-10-31 | INT-83 | `86ce2d8`+ | Not recorded | PASS (host, build) | [2026-10-31_INT-83/](logs/2026-10-31_INT-83/) | Continuous FOLLOW wheel targets |
| 2026-10-31 | INT-84 | `86ce2d8` | Not recorded | PASS (host, build) | [2026-10-31_INT-84/](logs/2026-10-31_INT-84/) | Navigation wheel selection and IR U-turn |
| 2026-10-31 | INT-85 | `86ce2d8` | Not recorded | PASS (host, build) | [2026-10-31_INT-85/](logs/2026-10-31_INT-85/) | 70 mm turn approach and immediate curve authority |
| 2026-10-31 | INT-86 | `86ce2d8`+ | Not recorded | PASS (host, build) | [2026-10-31_INT-86/](logs/2026-10-31_INT-86/) | Gentler steering transitions |
| 2026-10-31 | INT-87 | `86ce2d8`+ | Codex | PASS (host, build) | [2026-10-31_INT-87/](logs/2026-10-31_INT-87/) | Faster tracking trial |
| 2026-10-31 | INT-88 | `86ce2d8`+ | Not recorded | PASS (host, build) | [2026-10-31_INT-88/](logs/2026-10-31_INT-88/) | Gentler curve authority |
| 2026-10-31 | INT-89 | `86ce2d8`+ | Not recorded | PASS (host) | [2026-10-31_INT-89/](logs/2026-10-31_INT-89/) | IR curve response and straight release |
| 2026-10-31 | INT-90 | `86ce2d8`+ | Not recorded | PASS (host) | [2026-10-31_INT-90/](logs/2026-10-31_INT-90/) | Gentler entry and tighter sustained curves |
| 2026-10-31 | INT-91 | `86ce2d8`+ | Not recorded | PASS (host) | [2026-10-31_INT-91/](logs/2026-10-31_INT-91/) | Curve authority and guided straight exits |
| 2026-10-31 | INT-92 | `86ce2d8` | Not recorded | PASS (host, build) | [2026-10-31_INT-92/](logs/2026-10-31_INT-92/) | Smooth applied steering and pivot curves |
| 2026-10-31 | INT-93 | `86ce2d8` | Not recorded | PASS (build) | [2026-10-31_INT-93/](logs/2026-10-31_INT-93/) | IR calibration diagnostic preparation. No GPIO or physical check |
| 2026-10-31 | INT-94 | `86ce2d8`+ | Not recorded | PASS (host, dashboard) | [2026-10-31_INT-94/](logs/2026-10-31_INT-94/) | Encoder PID diagnostics and digital IR setup |
| 2026-11-01 | INT-95 | `b0c3d1b`+ | Codex | PASS (host, build) | [2026-11-01_INT-95/](logs/2026-11-01_INT-95/) | DOUT smooth navigation. Physical commissioning pending |
| 2026-11-01 | INT-96 | `b0c3d1b`+ | Codex | Failed before fix, then PASS (host, build) | [2026-11-01_INT-96/](logs/2026-11-01_INT-96/) | Straight junction continuation. Physical retest pending |
| 2026-11-01 | INT-97 | In manifest | Nicholas | PASS (host) | [2026-11-01_INT-97/](logs/2026-11-01_INT-97/) | Wheel separation and crossbar measurements |
| 2026-11-01 | INT-98 | `b0c3d1b`+ | Nicholas, Claude | PASS (host) | [2026-11-01_INT-98/](logs/2026-11-01_INT-98/) | IR spot measurements and curve steering hold. Not flashed |
| 2026-11-01 | INT-99 | `b0c3d1b`+ | Nicholas, Claude | MEASURED | [2026-11-01_INT-99/](logs/2026-11-01_INT-99/) | Wheel speed at a 200 mm/s request. Raised wheels 200 mm/s at 22-24% duty; floor 203-206 mm/s at 28-30% |
| 2026-11-01 | INT-100 | image `5e2a7ed4` | Nicholas, Claude | MEASURED; PASS (host) | [2026-11-01_INT-100/](logs/2026-11-01_INT-100/) | Wheel feedforward fit. Raised wheels at 60, 100, 140 mm/s; feedforward fitted |
| 2026-11-01 | INT-101 | image `bedea7c4` | Nicholas, Claude | MEASURED | [2026-11-01_INT-101/](logs/2026-11-01_INT-101/) | 100 mm/s step with fitted feedforward. Raised 100 mm/s step, peak 112 mm/s; wheels alternate below 60 mm/s |
| 2026-11-01 | INT-102 | image `16c6ae02` | Nicholas, Claude | MEASURED; PASS (host) | [2026-11-01_INT-102/](logs/2026-11-01_INT-102/) | Lower wheel KP. KP 0.0001 removes the alternation; raised wheels only |
| 2026-11-01 | INT-103 | image `b4dd145f` | Nicholas, Claude | OBSERVED | [2026-11-01_INT-103/](logs/2026-11-01_INT-103/) | First floor tracking with tuned wheels. 55 mm/s floor, two runs; run 1 lost the line at the crossing |
| 2026-11-01 | INT-104 | `b0c3d1b`+ | Nicholas, Claude | PASS (host) | At tag | Crossing heading restore. Not run |
| 2026-11-01 | INT-105 | image `23808dbd` | Nicholas, Claude | PASS at 55 mm/s | [2026-11-01_INT-105/](logs/2026-11-01_INT-105/) | Floor run with crossing heading restore. This course section only |
| 2026-11-01 | INT-106 | `89bfc65` | Nicholas, Claude | FAIL at 80 mm/s | [2026-11-01_INT-106/](logs/2026-11-01_INT-106/) | Tracking at 80 mm/s |
| 2026-11-01 | INT-107 | `89bfc65`+ | Nicholas, Claude | FAIL; PASS (host) | [2026-11-01_INT-107/](logs/2026-11-01_INT-107/) | Distance-scaled trial and PI steering. Scaling by 80/55 did not fix it |
| 2026-11-01 | INT-108 | `89bfc65`+ | Nicholas, Claude | PASS at 55 mm/s; FAIL at 80 mm/s | [2026-11-01_INT-108/](logs/2026-11-01_INT-108/) | PI floor runs and curve skew. Lost the line on the curve at 80 |
| 2026-11-01 | INT-109 | `89bfc65`+ | Nicholas, Claude | FAIL at 80 mm/s; PASS (host) | [2026-11-01_INT-109/](logs/2026-11-01_INT-109/) | Curve speed cap. 55 mm/s runs completed |
| 2026-11-01 | INT-110 | image `42761bd2` | Nicholas, Claude | FAIL; PASS (host) | [2026-11-01_INT-110/](logs/2026-11-01_INT-110/) | Curve speed latch. Undershoots and goes wide on curves |
| 2026-11-01 | INT-111 | image `a81f99bd` | Nicholas, Claude | FAIL; PASS (host) | [2026-11-01_INT-111/](logs/2026-11-01_INT-111/) | Same speed on curves, faster integral trial. Still wide on curves |
| 2026-11-01 | INT-112 | image `c552e3fc` | Nicholas, Claude | FAIL | [2026-11-01_INT-112/](logs/2026-11-01_INT-112/) | Integral step 6 at 80 mm/s. Stiff on straights; curve not held |
| 2026-11-01 | INT-113 | image `7b2645a6` | Nicholas, Claude | FAIL | — | 60 and 65 mm/s, 100 mm/s tuning plan. Full route fails at 60 and 65 mm/s |
| 2026-11-01 | INT-114 | image `0dec93e7` | Nicholas, Claude | FAIL | — | 100 mm/s baseline and slew. Curve not strong enough at 100 mm/s |
| 2026-11-01 | INT-115 | image `55902329` | Nicholas, Claude | FAIL | — | Bigger kick and 10 ms IR at 100 mm/s. Best so far, recovers by the end of the curve |
| 2026-11-01 | INT-116 | image `84411920` | Nicholas, Claude | FAIL on curves; PASS (host) | [2026-11-01_INT-116/](logs/2026-11-01_INT-116/) | Sensors moved back, slower integral unwind. Straights and crossing pass at 100 mm/s |
| 2026-11-01 | INT-117 | image `bbbe0ce4` | Nicholas, Claude | FAIL | [2026-11-01_INT-117/](logs/2026-11-01_INT-117/) | Slow unwind on three curves. First curve good, second wider, third off the line |
| 2026-11-01 | INT-118 | image `04a3ee45` | Nicholas, Claude | PASS on curves; FAIL on curve exit; PASS (host) | [2026-11-01_INT-118/](logs/2026-11-01_INT-118/) | Curves hold, curve-exit drain. 100 mm/s, these two trials |
| 2026-11-01 | INT-119 | image `cb4f9424` | Nicholas, Claude | FAIL; PASS (host) | [2026-11-01_INT-119/](logs/2026-11-01_INT-119/) | Curve exit swings the other way. Swings the opposite way and loses the line |
| 2026-11-01 | INT-120 | image `1ed81850` | Nicholas, Claude | FAIL on S-bend; PASS (host) | [2026-11-01_INT-120/](logs/2026-11-01_INT-120/) | S-bend after the exit hold |
| 2026-11-01 | INT-121 | image `7feb06e1` | Nicholas, Claude | FAIL | [2026-11-01_INT-121/](logs/2026-11-01_INT-121/) | Instant exit reset hurts curves. Curves veer off, worse than the previous trial |
| 2026-11-01 | INT-122 | image `c5d9fe1e` | Nicholas, Claude | FAIL; PASS (host) | [2026-11-01_INT-122/](logs/2026-11-01_INT-122/) | Hold limit and hump halving |
| 2026-11-01 | INT-123 | image `be6539c9` | Nicholas, Claude | PASS on straights and curves; FAIL at one junction; PASS (host) | [2026-11-01_INT-123/](logs/2026-11-01_INT-123/) | 100 mm/s defaults from trial o. 100 mm/s; S-bend not separately checked |
| 2026-11-01 | INT-124 | `159e89b`+ | Nicholas, Claude | PASS (host) with 2 known failures | At tag | Raw-input line replay tests. No physical run |
| 2026-11-02 | INT-125 | `159e89b`+ | Nicholas, Claude | PASS (host, build) | At tag | Line-trace diagnostics build |
| 2026-11-02 | INT-126 | `159e89b`+ | Nicholas, Claude | PASS (host, build), 1 known failure | At tag | Distance-based steering law |
| 2026-11-02 | INT-127 | `159e89b`+ | Nicholas, Claude | PASS (host, build) | At tag | Line loss and reacquisition evidence. No known failures left |
| 2026-11-02 | INT-128 | `159e89b`+ | Nicholas, Claude | PASS (host, build) | At tag | Pivot junction turns |
| 2026-11-02 | INT-129 | `159e89b`+ | Nicholas, Claude | PASS (host, build) | At tag | 100 mm/s course speed guardrail |
| 2026-11-02 | INT-130 | `1161a5c`+ | Nicholas, Claude | PASS (host, build) | At tag | Subsystem rename and file merge |
| 2026-11-02 | INT-131 | `49effa4`+ | Nicholas, Claude | PASS (host, build) | [2026-11-02_INT-131/](logs/2026-11-02_INT-131/) | One header per subsystem |
| 2026-11-02 | INT-132 | `79c13f1` | Nicholas, Claude | FAIL | [2026-11-02_INT-132/](logs/2026-11-02_INT-132/) | Lower steering after overshoot. Veered off the track; no line trace captured |
| 2026-11-02 | INT-133 | Not recorded | Nicholas, Claude | FAIL | [2026-11-02_INT-133/](logs/2026-11-02_INT-133/) | Speed stuck at half on the trial image. Stuck at 50 mm/s: hump encounter held open |
| 2026-11-02 | INT-134 | INT-133 image | Nicholas, Claude | OBSERVED | — | Half speed on humps removed. Tracking good at 50 mm/s; half-speed hump request removed |
| 2026-11-02 | INT-135 | INT-134 image | Nicholas, Claude | PASS tracking at 100 mm/s; FAIL on stops | [2026-11-02_INT-135/](logs/2026-11-02_INT-135/) | False line loss on straights. `LINE_LOST_MM` 200 too short, set to 350 |
| 2026-11-02 | INT-136 | INT-135 image | Nicholas, Claude | PASS on straights and curves; FAIL at junctions | [2026-11-02_INT-136/](logs/2026-11-02_INT-136/) | Pivot ends blind past the branch. Robot did not turn to the next branch |
| 2026-11-02 | INT-137 | INT-136 image | Nicholas, Claude | PASS on those turns; FAIL on barcodes | — | Barcode decoder rework. Barcodes not read at all |
| 2026-11-02 | INT-138 | INT-137 image | Nicholas, Claude | FAIL | — | Encoder stops and barcode visibility. Barcodes not read; robot sometimes stops |
| 2026-11-02 | INT-139 | `79c13f1`+ | Codex | PASS (host, dashboard) | [2026-11-02_INT-139/](logs/2026-11-02_INT-139/) | Subsystem sensor dashboards |
| 2026-11-02 | INT-139b | image `27953ce2` | Nicholas, Claude | PASS one loop; FAIL at junction | [2026-11-02_INT-139/](logs/2026-11-02_INT-139/) | Blind exit from a straight crossing |
| 2026-11-02 | INT-140 | `79c13f1`+ | Codex | PASS (host, dashboard) | [2026-11-02_INT-140/](logs/2026-11-02_INT-140/) | Compact simultaneous subsystem overview |
| 2026-11-02 | INT-140b | Not recorded | Nicholas, Claude | FAIL | [2026-11-02_INT-140/](logs/2026-11-02_INT-140/) | Barcode module misses narrow gaps. Passes the barcode with no read |
| 2026-11-02 | INT-141 | Not recorded | Nicholas | OBSERVED | — | Printed barcode black reads weakly. Hand check: many gaps missed; printed black reads weaker than line black |
| 2026-11-02 | INT-141b | image `2c51c247` | Nicholas, Claude | FAIL | [2026-11-02_INT-141/](logs/2026-11-02_INT-141/) | Barcode after recalibration. Barcode not sensed; tracking and crossings ran |
| 2026-11-02 | INT-142 | Not recorded | Nicholas, Claude | Host and build only | — | Barcode edge count in the trace. Trace gains a running count of GP16 edges |
| 2026-11-02 | INT-143 | image `45f1acad` | Nicholas, Claude | FAIL | [2026-11-02_INT-143/](logs/2026-11-02_INT-143/) | Barcode module output stays solid |
| 2026-11-02 | INT-144 | Not recorded | Nicholas | OBSERVED | — | Barcode module LED responds slowly. Module LED slow by hand; not timed |
| 2026-11-02 | INT-145 | image `45f1acad` | Nicholas, Claude | FAIL | [2026-11-02_INT-145/](logs/2026-11-02_INT-145/) | New barcode module, chatter. Barcode A not scanned; module works |
| 2026-11-02 | INT-146 | image `719a01ba` | Nicholas, Claude | FAIL | — | Raw barcode widths. Barcode still not read |
| 2026-11-02 | INT-147 | image `24b25fb5` | Nicholas, Claude | FAIL | [2026-11-02_INT-147/](logs/2026-11-02_INT-147/) | Barcode capture polarity bug. No read; the capture handler has no host test |
| 2026-11-02 | INT-148 | image `84f8e2c0` | Nicholas, Claude | FAIL | [2026-11-02_INT-148/](logs/2026-11-02_INT-148/) | Filtered barcode widths. No read |
| 2026-11-02 | INT-149 | image `84f8e2c0` | Nicholas, Claude | OBSERVED | [2026-11-02_INT-149/](logs/2026-11-02_INT-149/) | Barcode threshold moved toward white. No barcode event; pass looks like A with wide bars only (inferred) |
| 2026-11-02 | INT-150 | Not recorded | Nicholas, Claude | OBSERVED | — | Narrow bars are seen. Narrow bar about 5 mm and seen by the LED; `BARCODE_MIN_ELEMENT_COUNTS` 23 |
| 2026-11-02 | INT-151 | image `c9cdb44a` | Nicholas, Claude | PASS, one read | [2026-11-02_INT-151/](logs/2026-11-02_INT-151/) | First barcode read. One print, one direction |
| 2026-11-02 | INT-152 | Not recorded | Nicholas, Claude | OBSERVED | — | Barcode misses after the first read. One read in three tries; `BARCODE_MIN_ELEMENT_COUNTS` 16 |
| 2026-11-02 | INT-153 | image `0e93c6a4` | Nicholas, Claude | PASS two barcode reads; FAIL at junction | — | Queued turn stopped on onset skew. Stopped with a queued turn |
| 2026-11-02 | INT-154 | `d97e3d9` | Nicholas, Claude | PASS course run; FAIL on hump | — | Hump not reported |
| 2026-11-02 | INT-155 | `d97e3d9`+ | Nicholas, Claude | FAIL | [2026-11-02_INT-155/](logs/2026-11-02_INT-155/) | Barcode lost after a hump, hump not counted |
| 2026-11-02 | INT-156 | `d97e3d9`+ | Nicholas, Claude | PASS barcode; FAIL on hump | [2026-11-02_INT-156/](logs/2026-11-02_INT-156/) | Hump detector replaced with roll. All letters and a flipped print not tested |
| 2026-11-02 | INT-157 | `d97e3d9`+ | Nicholas, Claude | FAIL | [2026-11-02_INT-157/](logs/2026-11-02_INT-157/) | Humps counted on the wrong encounters. Hump count wrong |
| 2026-11-02 | INT-158 | `d97e3d9`+ | Nicholas, Claude | FAIL | [2026-11-02_INT-158/](logs/2026-11-02_INT-158/) | Hump baseline moved by pivots. Tracking off course, not analysed; `humps=5` against 1 crossed |
| 2026-11-03 | INT-160 | `a7593b0` merged with `1288042`, plus fixes | Claude | PASS (host) | At commit | PR 149 host suite after fixing test order, a commented-out self-test and a triage script syntax error. No Arm build or Pico run; watchdog policy only |



## Audits, not tests

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
|---|---|---|---|---|---|---|
| 2026-10-29 | INT-11 | `4b00e20`+ | Codex | See check output | [INT-11_final.txt](logs/2026-10-29_INT-11_final.txt), [INT-11_checker.py](logs/2026-10-29_INT-11_checker.py) | Documentation validation. Markdown and static checks only |
| 2026-10-30 | INT-20 | `62ee522` | Not recorded | PASS (host, build, static); plan BLOCKED | [INT-20-completion.json](logs/2026-10-30_INT-20-completion.json) | Repository findings |
| 2026-10-30 | INT-22 | `62ee522`+ | Codex | PASS (checker) | [INT-21-completion.json](logs/2026-10-30_INT-21-completion.json), [INT-22-completion-check.txt](logs/2026-10-30_INT-22-completion-check.txt) | Completion record selection. Completion checker exits 1 on BLOCKED stages |
| 2026-10-31 | INT-44 | `9bcab80`+ | Codex | PASS (host, dashboard, build) | [2026-10-31_INT-44/](logs/2026-10-31_INT-44/) | Repository audit |
| 2026-10-31 | INT-45 | `2005c68`+ | Codex | PASS (host, dashboard, build) | [2026-10-31_INT-45/](logs/2026-10-31_INT-45/) | Repository audit fixes |
| 2026-10-31 | INT-46 | `5e63fe0` | Codex | PASS (host, dashboard) | [2026-10-31_INT-46/](logs/2026-10-31_INT-46/) | Repository audit. No source change |
| 2026-10-31 | INT-47 | `54e5ec4` | Codex | PASS (host, dashboard, build) | [2026-10-31_INT-47/](logs/2026-10-31_INT-47/) | Repository audit fixes |
| 2026-10-31 | INT-50 | `30ec622` | Codex | PASS (links); plan BLOCKED | [2026-10-31_INT-50/](logs/2026-10-31_INT-50/) | Consolidated completion tracking |
| 2026-10-31 | INT-51 | `fc9fa5d` | Codex | PASS (links); plan BLOCKED | [2026-10-31_INT-51/](logs/2026-10-31_INT-51/) | Retire the duplicate plan |
| 2026-11-02 | INT-159 | Not recorded | Claude | No run | [2026-11-02_raw-captures/](logs/2026-11-02_raw-captures/) | Raw captures moved. Gzipped; `int-158-run.txt` deleted, it held no run |
| 2026-11-03 | INT-161 | `e576760` | Claude | No run | — | Merge lineage. PR 148 merged INT-132 to INT-159 as `e576760`; each row keeps its original source and result, including INT-158 FAIL |
| 2026-11-03 | INT-162 | `7275347` | Claude | PASS (build) | — | Arm builds. Full course and test images with `WATCHDOG=0/1`; full course also with `TRACE=0/1`. Six images, one warning from the SDK's `mqtt.c`. Not a firmware or physical pass |
| 2026-11-03 | INT-163 | `7275347` | Claude | Analysis; cause not found | — | INT-157 capture, line 12673 on. LEFT event, advance, stop, then pivot from 97.11 s. Wheel targets go to zero at 97.79 s, then the next telemetry is a fresh boot (`mapped=NO`, about 7 s up). No `REACQUIRE` or `STOPPED` is captured. Telemetry has no pose, so it cannot show where the car left the line. Needs video or a repeat run |
| 2026-11-03 | INT-164 | `7275347` | Claude | Analysis; likely handling | — | INT-156 capture, lines 7806 to 8178. Before `SCAN`, wheels read up to 330 mm/s against targets of 76 and 124 while duty falls, with pitch from -28 to +32 degrees and front jumping from 378 to 3022 mm. This fits the car being lifted, not a scan fault. Not proven: stop cause is not telemetered |
| 2026-11-03 | INT-165 | `7275347` | Claude | Analysis; artefact in these cases | — | `L=1935 R=-222` (INT-157, line 11667) follows a capture gap from 74.05 to 76.21 s; `WHEEL` around it is normal. `L=895 R=898` (INT-154, line 28918) has the same pattern, with `WHEEL` at about 100 mm/s. `WHEEL` peak is 210 mm/s or less in INT-154, 155 and 157. No encoder fault in telemetry in the four captures; `HUMP` frames show `encoder_ok=0` 14, 3, 8 and 0 times, not explained. Does not explain earlier encoder-fault stops, which these captures do not hold |
| 2026-11-03 | INT-166 | `7275347` | Claude | Analysis; likely handling | — | `IMU calibration failed` (INT-154, lines 22501 to 26351). On that boot counts read `L=65531 R=65379`, about 5 and 157 counts backward, so the car moved during calibration. Firmware restarts sampling on motion, then times out at 5 s. Likely handling; not proven |
| 2026-11-03 | INT-167 | `909bfc2` | Claude | PASS (host) | — | Missing `EVENT BARCODE`. INT-154 shows `A->U_TURN` in telemetry with no event in the capture. Rule moved to `mission_barcode_event_needed`, with a host test; the test failed when the rule was broken. Needs a real run |
| 2026-11-03 | INT-168 | `7275347` | Claude | Analysis; open | — | Restarts. The Pico clock restarts 10 times across INT-154 to 157: 8 from READY, 1 from STOPPED and 1 from NAVIGATE (INT-157, line 12968, about 0.9 s after wheel targets reached zero). `TELEMETRY #0` is not a restart, because the sequence is 8 bits and wraps. Captures cannot show whether a restart was manual. Awaiting Nicholas |
| 2026-11-03 | INT-169 | `7275347` | Claude | Analysis; PASS (host) | — | No echo as a stop cause. `test_robot_data`, `test_safety` and `test_mission` already show it never stops a run. INT-154 stops at lines 21775 and 26121 follow the same both-black pattern, and only the first has no echo, so no echo is not their common cause. That stop's cause is still unknown |
| 2026-11-03 | INT-170 | `7275347` | Claude | Analysis; no change | — | Barcode capture overflow shown in 429 of 1033, 37 of 618, 127 of 572 and 64 of 639 telemetry rows (INT-154 to 157), nearly all while driving. Captures do not give reads per pass, so they cannot show that it loses reads. Ring stays at 64 pending the read-rate runs |
| 2026-11-03 | INT-171 | `7275347` | Claude | Analysis; host only | — | Hump detector stuck on. INT-156 and 157 hold `state=1` for up to 9.9 s, on images before the current roll detector. Current code ends an encounter at `HUMP_MAX_TRAVEL_MM`, with host cases. A real hump within that travel can still be missed. INT-122 and INT-133 hold no `HUMP` frames, so they cannot be replayed. Needs a car run |
| 2026-11-03 | INT-172 | `4037b48` | Claude | Audit; no defect (source only) | — | D14 core-0 audit, recorded in `docs/architecture.md`. Contention, lock duration and load not measured; they need the Pico |
| 2026-11-03 | INT-173 | `4037b48` | Claude | MEASURED (target compiler) | — | `sizeof_probe.sh`: `IrTracker` 20 bytes, `LineIrSnapshot` 24 bytes; `IrRawWord` uses 3 bits. Data table updated |
| 2026-11-03 | INT-174 | `4037b48` | Claude | PASS (build, host); MEASURED (build) | — | Arm builds repeated, six images. Full-course image `2fd47223`: text 387800, data 424, bss 84496. Host suite, dashboard, CLI, HTTP and field-tool tests pass. Latest build, not the final image; not a firmware or physical pass |
| 2026-11-03 | INT-175 | `02d30fa` | Nicholas, Claude | User report | — | INT-168 restarts: Nicholas reports the battery was not connected properly, a hardware fix. Closed; no firmware cause investigated |
| 2026-11-03 | INT-176 | `ebb3a2c` | Claude | PASS (host, build) | — | Stall guard on for `FULL_COURSE=1`, by Nicholas. Replay of the rule on INT-154 to 157 `WHEEL` frames found one false stop (INT-156, line 1614: target 3 mm/s, wheel still for 1060 ms), so targets under 10 mm/s no longer count. The next highest still target was 6.6 mm/s. The floor is provisional. Full-course image has no car run |
| 2026-11-03 | INT-177 | `ebb3a2c` | Claude | MEASURED (target compiler) | — | Application stored data: 2076 bytes static across `app_program` objects; `Mission` 212 bytes, `bypass_control_t` 72, `MotionController` 60. `TRACE=0` saves 464 bytes bss. Bit-reduction estimates are in `pending.md`; nothing built |
| 2026-11-03 | INT-178 | `c2b0915` | Claude | PASS (host, build); MEASURED (build) | — | Bit cuts on `chore/resolve-pending` only. Six Arm images build; full-course image `dbf8acdf`: text 388856, data 424, bss 84316 against 387800, 424, 84496 at INT-174. On target: `Mission` 212 to 164 bytes, `MotionController` 60 to 44, `IrTracker` 20 to 8, `LineIrSnapshot` 24 to 12, `ObstacleProfile` 14 to 9, `ObstacleRay` 4 to 3; telemetry 23 to 22 bytes. Host, dashboard, CLI, HTTP and field-tool tests pass. Not a firmware or physical pass; no car run |
| 2026-11-03 | INT-179 | `3c102325519afa6ba36615b96c0a30f49e89b7f8` | Codex | PASS (host only) | — | `testing/pure/run.sh`: existing full suite, enabled/disabled stall guard, 5 million snapshot reads, 300000 frame stress and 100000 mapping exchanges; no robot or network execution |
| 2026-11-03 | INT-180 | `3c102325519afa6ba36615b96c0a30f49e89b7f8` | Codex | PASS (Arm build only) | [profile measurements](logs/2026-11-03_INT-180-build-matrix.json) | `nic2_wd0_trace0_test1_course0`; text/data/bss 385132/424/84044 bytes; image hash in measurements; not flashed |
| 2026-11-03 | INT-181 | `3c102325519afa6ba36615b96c0a30f49e89b7f8` | Codex | PASS (Arm build only) | [profile measurements](logs/2026-11-03_INT-180-build-matrix.json) | `nic2_wd0_trace1_test1_course0`; text/data/bss 387492/424/84508 bytes; image hash in measurements; not flashed |
| 2026-11-03 | INT-182 | `3c102325519afa6ba36615b96c0a30f49e89b7f8` | Codex | PASS (Arm build only) | [profile measurements](logs/2026-11-03_INT-180-build-matrix.json) | `nic2_wd1_trace0_test1_course0`; text/data/bss 385616/424/84064 bytes; image hash in measurements; not flashed |
| 2026-11-03 | INT-183 | `3c102325519afa6ba36615b96c0a30f49e89b7f8` | Codex | PASS (Arm build only) | [profile measurements](logs/2026-11-03_INT-180-build-matrix.json) | `nic2_wd1_trace1_test1_course0`; text/data/bss 387980/424/84528 bytes; image hash in measurements; not flashed |
| 2026-11-03 | INT-184 | `3c102325519afa6ba36615b96c0a30f49e89b7f8` | Codex | PASS (Arm build only) | [profile measurements](logs/2026-11-03_INT-180-build-matrix.json) | `nic2_wd0_trace1_test0_course1`; text/data/bss 387764/424/84496 bytes; image hash in measurements; not flashed |
| 2026-11-03 | INT-185 | `a5157f9d0461094ce2a30aac64b8671b2f6e77e9` | Codex | PASS (host only) | — | Eight field-tool cases: unavailable speeds preserve state transitions, malformed traces rejected, no unsupported lap PASS, globally unique IDs and lighting-separated samples |
| 2026-11-03 | INT-186 | analysis tool `a5157f9d0461094ce2a30aac64b8671b2f6e77e9`; captures retain INT-154/156/157 lineage | Codex | AUDIT (no robot run) | [capture review](logs/2026-11-03_INT-186-capture-review.md) | Acquisition-time speeds compared with arrival-time estimates; physical root causes not inferred |
| 2026-11-04 | INT-187 | `7275347`+ | Claude | PASS (host, build) | At commit | Crossing exit steering. INT-154 and INT-156 traces: steer 90 on 23 of 24 straight crossing exits, median heading swing about 7 to 9 degrees. Trailing sensor ignored until clear, no false bend. Image `e10c2767` not flashed; no physical run, jerk not yet shown fixed |
| 2026-11-04 | INT-188 | INT-187 image `e10c2767` inferred from build and capture times | Nicholas, Claude | FAIL at junctions | [2026-11-04_INT-188/](logs/2026-11-04_INT-188/) | Veered left at straight crossings. Both black under 9 mm, so no commit and the INT-187 fix never ran; left-only steer 60 turned about 20 degrees at t=25960 |
| 2026-11-04 | INT-189 | `7275347`+ | Claude | PASS (host, build) | At commit | Straight crossing uses the ramped advance, 36 mm, then tracks; candidate travel 6 mm. Image `770123ac` not flashed; no physical run. Short both-black reads are unlabelled, so a false commit on a curve is not ruled out |
| 2026-11-04 | INT-190 | INT-189 image `770123ac` inferred from the advance in the trace | Nicholas, Claude | PASS two crossings committed; FAIL after them | [2026-11-04_INT-190/](logs/2026-11-04_INT-190/) | Veered left after the crossing. First touch about 40 mm passed the bend limit: steer -90, then lean -30 for 98 mm, heading -11 to -36 degrees. Left stayed black 27 to 29 mm of the advance, right cleared early; sensor imbalance not bench checked |
| 2026-11-04 | INT-191 | `7275347`+ | Claude | PASS (host, build) | At commit | First touch after a crossing is a bend only past 100 mm. Image not flashed; no physical run |
| 2026-11-04 | INT-192 | Saved captures, no new run | Nicholas, Claude | OBSERVED | [2026-11-04_INT-190/](logs/2026-11-04_INT-190/), [raw captures](logs/2026-11-02_raw-captures/) | Left IR clears crossbars late. Both go black together (median onset within 2 mm); left clears after right on 99 of 100 low-skew crossings, median about 14 mm with exit steering, 27 to 30 mm on 2 straight advances. Nicholas reports the potentiometer does not remove it. Cause not isolated: no module swap or slow hand test yet |
| 2026-11-04 | INT-193 | `cf1a3c2` | Claude | PASS (host, build); MEASURED (build) | — | `chore/centralise-pending`: all unmerged branches together. Host suite passes. Seven Arm images build, one warning from the SDK's `mqtt.c`; `TERRAIN_DIAGNOSTICS=1` image also builds. Full-course image `76e62048`: text 388412, data 424, bss 84272. `Mission` 160 bytes. Not flashed; no firmware or physical pass |
| 2026-11-04 | INT-194 | `6839d26` plus the pause test fix in the next commit | Claude | PASS (host, build); MEASURED (build) | — | Nicholas's decisions: `COURSE_SPEED_MMPS` 100 for course and bypass, `full_course` at `TRACE=0`, `flash.sh trial`, 200 ms U-turn pause. Host suite passes at 100 mm/s and at a temporary 80 mm/s. Eight Arm images build. `full_course` profile `b3a7a5eb`: text 386104, data 424, bss 83808; with `TRACE=1` `3db86926`: text 388408, bss 84272, so `TRACE=0` saves 464 bytes of RAM. Not flashed; bypass at 100 mm/s has never run |

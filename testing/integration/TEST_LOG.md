# Integration test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)
- [Audits, not tests](#audits-not-tests)

## Test cases

| ID | Scope | Method | Expected |
| --- | --- | --- | --- |
| INT-03 | line_barcode request / Motion stop and expiry / health | Host actual Motion service with fake kernel, normal and TEST profiles | Stop independent of mutex; expired commands latch; reset cannot replay; TEST inhibits outputs |
| INT-04 | Reproducible template build and ownership | Normal and TEST=1 SMP builds; upstream byte comparison and static audit | Both UF2s; unchanged runtime/build source; init-before-start; native ADC disabled; no app network calls |
| INT-05 | Real line_barcode task and unchanged Motion API contract | Host task_ir with fake GPIO/ADC/Motion, normal and TEST profiles | Recovery remains enabled; acquisition time preserved; GP21/reset/start, failed init/acquisition/health/publish/reset, inhibition and missed release fail closed |
| INT-06 | Read-only MQTT service | Actual mqtt_utk.c with fake lwIP/kernel/clock; 25 reconnect cycles | QoS0/nonretained; 500 ms stream; 3 s refusal/immediate-failure/alloc retry; skip unavailable/oversized/failed publication; latest reconnect; no timer accumulation |
| INT-07 | Telemetry isolation from line_barcode and Motion | Actual line_barcode loop with fake hardware/Motion/kernel, MQTT enabled, normal and TEST; snapshot available or always busy | GP21 dominates; reset/start/recovery/watchdog and timestamps unchanged; failed telemetry never authorizes motion |
| INT-09 | Wireless/offline build and preservation | Normal/TEST MQTT and offline normal builds, source/Word/ownership audit | All UF2s compile; Motion/control policy, kernel and historical sources preserved; credentials excluded |

Later cases are named in their result rows.

## Results

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-08 | INT-03 | logs/2026-10-08_source-sha256.json | Codex | PASS (host) | at tag | Fake kernel, not scheduler/physical proof |
| 2026-10-08 | INT-03/05 | logs/2026-10-08_s3-proportional-source-sha256.json | Codex | PASS (host) | host log | Real line_barcode loop with fake inputs/Motion and unchanged real Motion service with fake kernel; both profiles |
| 2026-10-08 | INT-03/05/06/07 | Not recorded | Codex | PASS (host) | — | Fake kernel/hardware/network; unchanged Motion actual service also rerun in both profiles |
| 2026-10-08 | INT-04 | Same manifest; base HEAD in JSON | Codex | PASS (build) | logs/2026-10-08_migration-evidence.json; at tag | Both profiles compiled; no firmware flashing |
| 2026-10-08 | INT-04 | Same manifest; base HEAD in JSON | Codex | PASS (build) | normal build; TEST build | Both UF2s; 564 protected files unchanged; no firmware flashing |
| 2026-10-08 | INT-09 | Same manifest and base HEAD | Codex | PASS (build) | normal MQTT build; TEST MQTT build; offline build | Built without flashing; private config content excluded; unrelated line_barcode formatting/comment edits left uncommitted |
| 2026-10-08 | INT-09 | `57f3b95`+ | Codex | PASS (static) | — | Integrated concurrent documentation update; 160 local links valid; runtime unchanged. Later user driver formatting is token-equivalent to the tested source; no new hardware claim |
| 2026-10-09 | INT-09 | Not recorded | Codex | PASS | wireless TEST, wireless normal, offline TEST, offline normal, strict compile | Comms reporting/service restored; Motion–Obstacle, app startup/shared header, kernel and Make source unchanged; example credentials only; no flashing |
| 2026-10-29 | INT-12 | `fe39e3a` | Codex | PASS (host, build) | — | Restructure stage #130: stop, expiry and reset policy |
| 2026-10-29 | INT-13 | `b862817` | Codex | PASS (host, build) | — | Stage #131: seven states, action IDs, recovery |
| 2026-10-29 | INT-14 | `ab54479` | Codex | PASS (host, build) | — | Stage #132: mapping exchanges, event retry, Terrain faults |
| 2026-10-29 | INT-15 | `407cbfa` | Codex | PASS (host, build) | — | PASS (host, static, build). Stage #133: tasks, mutexes, motor writer, no app networking |
| 2026-10-29 | INT-16 | `0448890`+ | Codex | PASS (host, build) | — | PASS (host, static, build). Production gates, 150 mm trigger, synthetic profile values |
| 2026-10-29 | INT-17 | `0448890`+ | Codex | BLOCKED | — | Full-plan audit; hardware unavailable |
| 2026-10-29 | INT-18 | `0448890`+ | Codex | RETIRED (build) | At tag | Container tooling, removed in `73c8a93`. Tooling checks passed; wireless TEST build failed on undefined `BYPASS_REACQUIRE` |
| 2026-10-29 | INT-19 | `0448890`+ | Codex | PASS (host, build) | — | FAIL, then PASS (host, build). PR integration gate. First host compile failed on removed bypass fields; passed after integration fixes |
| 2026-10-30 | INT-21 | `62ee522`+ | Codex | PASS (host, build) | — | Scan ID regression and cleanup |
| 2026-10-30 | INT-23 | `62ee522`+ | Codex | PASS (host) | — | Rear-edge geometry |
| 2026-10-30 | INT-24 | `62ee522`+ | Codex | PASS (host, build) | — | PASS (host, build, static); plan BLOCKED. Resource and binary-protocol audit. Also SYS-12 |
| 2026-10-30 | INT-25 | `62ee522` | Codex | PASS (host, build) | — | PASS (host, build, static); plan BLOCKED. Event contention and unused-path cleanup |
| 2026-10-30 | INT-26 | `cc3edb3`+ | Codex | PASS (build) | At tag | Recovery stop diagnostics. Hardware diagnosis pending |
| 2026-10-30 | INT-27 | `cc3edb3` | Codex | PASS (host, build) | — | Recovery stall tuning. Physical recovery pending |
| 2026-10-30 | INT-28 | `cc3edb3` | Codex | PASS (host, build) | — | Sustained line-loss qualification. Physical run pending |
| 2026-10-30 | INT-29 | `cc3edb3` | Codex | PASS (build) | — | Recovery failure inputs. Physical failure attribution pending |
| 2026-10-30 | INT-30 | `cc3edb3` | Codex | PASS (host, build) | — | Bug reproduced, then PASS (host, build). Forward FOLLOW slowdown. Physical run pending |
| 2026-10-30 | INT-31 | `cc3edb3` | Codex | PASS (host, build) | — | Recovery wheel-speed range. Physical sweep pending |
| 2026-10-30 | INT-32 | `cc3edb3` | Nicholas | PASS (host, build) | — | Stall configuration integration. Nicholas confirmed the default-disabled stall guard |
| 2026-10-30 | INT-35 | `8be851b`+ | Codex | PASS (host, build) | — | Stationary wider recovery. No physical reacquisition |
| 2026-10-30 | INT-37 | `8be851b` | Codex | PASS (host, build) | — | Bug reproduced, then PASS (host, build). Junction speed-controller reversal. Software reversal found; not shown to be the physical vibration |
| 2026-10-30 | INT-38 | `8be851b` | Codex | PASS (host, build) | — | Bug reproduced, then PASS (host, build). Turn endpoint reactivation. Cause of physical mid-turn jitter not established |
| 2026-10-30 | INT-39b | In manifest | Nicholas | PASS (host, build) | — | Remove fixed junction advance. Result recorded under S1-U21; no physical junction check |
| 2026-10-31 | INT-42 | `9bcab80`+ | Codex | PASS (host, build) | — | Toggleable binary steering buildup. No physical steering check |
| 2026-10-31 | INT-49 | `545a6a3`+ | Codex | PASS (host, build) | [2026-10-31_INT-49/](logs/2026-10-31_INT-49/) | PASS (host, dashboard, build). Barcode branch integration. At `545a6a3`; no physical barcode check |
| 2026-10-31 | INT-52 | `fc9fa5d` | Codex | FAIL (host) | [2026-10-31_INT-52/](logs/2026-10-31_INT-52/) | Recovery brake audit. Recovery handoff bug reproduced on host |
| 2026-10-31 | INT-54 | `8981d04`+ | Codex | PASS (host) | [2026-10-31_INT-54/](logs/2026-10-31_INT-54/) | Recovery brake handoff fix |
| 2026-10-31 | INT-55 | `8981d04`+ | Codex | PASS (host, build) | [2026-10-31_INT-55/](logs/2026-10-31_INT-55/) | Stable visible recovery handoff |
| 2026-10-31 | INT-56 | `8981d04`+ | Codex | PASS (host) | [2026-10-31_INT-56/](logs/2026-10-31_INT-56/) | NAVIGATE jitter audit. Audit assertions; physical jitter not proven |
| 2026-10-31 | INT-57 | `8981d04`+ | Codex | PASS (host) | [2026-10-31_INT-57/](logs/2026-10-31_INT-57/) | Line tracking lifecycle audit. Audit assertions; no source change |
| 2026-10-31 | INT-58 | `8981d04`+ | Codex | PASS (host) | [2026-10-31_INT-58/](logs/2026-10-31_INT-58/) | Failed as expected, then PASS (host). Wider repeating recovery sweep |
| 2026-10-31 | INT-59 | `8981d04`+ | Codex | PASS (host) | [2026-10-31_INT-59/](logs/2026-10-31_INT-59/) | Straight NAVIGATE slowdown and jitter audit. Audit assertions; no source change |
| 2026-10-31 | INT-60 | `8981d04`+ | Codex | PASS (host) | [2026-10-31_INT-60/](logs/2026-10-31_INT-60/) | Navigation trigger and low-speed transition check. Synthetic transitions only |
| 2026-10-31 | INT-61 | `8981d04`+ | Codex | PASS (host, build) | [2026-10-31_INT-61/](logs/2026-10-31_INT-61/) | Full-speed 25 mm straight junction exit |
| 2026-10-31 | INT-62 | `8981d04`+ | Codex | PASS (host, build) | [2026-10-31_INT-62/](logs/2026-10-31_INT-62/) | Immediate simple 70 degree recovery |
| 2026-10-31 | INT-63 | `8981d04`+ | Codex | PASS (host, build) | [2026-10-31_INT-63/](logs/2026-10-31_INT-63/) | 500 ms recovery takeover qualification |
| 2026-10-31 | INT-64 | In manifest | Codex | PASS (host) | [2026-10-31_INT-64/](logs/2026-10-31_INT-64/) | Short physical sweep angle investigation. Estimated angle only |
| 2026-10-31 | INT-69 | `4365e10`+ | Codex | PASS (host) | — | Digital IR tracking and half-speed navigation |
| 2026-10-31 | INT-72 | `4365e10`+ | Codex | PASS (host) | — | Stronger digital steering and junction speed ramp |
| 2026-10-31 | INT-73 | `4365e10`+ | Codex | PASS (host) | — | Reverse digital tracking correction |
| 2026-10-31 | INT-74 | `4365e10`+ | Codex | PASS (host) | — | Diagnose GP20 start while Mission displays READY. Firmware diagnosis pending |
| 2026-10-31 | INT-75 | In manifest | Codex | PASS (host) | — | Verify digital IR through Mission and wheel outputs |
| 2026-10-31 | INT-76 | In manifest | Codex | PASS (host, build) | — | Execute queued direction on first fresh both-black sample |
| 2026-10-31 | INT-77 | In manifest | Codex | PASS (host, build) | — | Reverse mounted IR steering correction. User reported the physical steering direction |
| 2026-10-31 | INT-78 | In manifest | Codex | PASS (host) | — | Single-wheel 1.5x IR correction |
| 2026-10-31 | INT-79 | `86ce2d8`+ | Codex | PASS (host) | — | Reported IMU calibration. Not a physical calibration |
| 2026-10-31 | INT-80 | `86ce2d8`+ | Not recorded | PASS (host, build) | — | Smooth tracking with strong curve correction |
| 2026-10-31 | INT-81 | `86ce2d8`+ | Not recorded | PASS (host, build) | — | Stronger sustained curves and modest speed increase |
| 2026-10-31 | INT-82 | `86ce2d8`+ | Not recorded | PASS (host, build) | — | Half-to-full curve speed ramp |
| 2026-10-31 | INT-83 | `86ce2d8`+ | Not recorded | PASS (host, build) | — | Continuous FOLLOW wheel targets |
| 2026-10-31 | INT-84 | `86ce2d8` | Not recorded | PASS (host, build) | — | Navigation wheel selection and IR U-turn |
| 2026-10-31 | INT-85 | `86ce2d8` | Not recorded | PASS (host, build) | — | 70 mm turn approach and immediate curve authority |
| 2026-10-31 | INT-86 | `86ce2d8`+ | Not recorded | PASS (host, build) | — | Gentler steering transitions |
| 2026-10-31 | INT-87 | `86ce2d8`+ | Codex | PASS (host, build) | — | Faster tracking trial |
| 2026-10-31 | INT-88 | `86ce2d8`+ | Not recorded | PASS (host, build) | — | Gentler curve authority |
| 2026-10-31 | INT-89 | `86ce2d8`+ | Not recorded | PASS (host) | — | IR curve response and straight release |
| 2026-10-31 | INT-90 | `86ce2d8`+ | Not recorded | PASS (host) | — | Gentler entry and tighter sustained curves |
| 2026-10-31 | INT-91 | `86ce2d8`+ | Not recorded | PASS (host) | — | Curve authority and guided straight exits |
| 2026-10-31 | INT-92 | `86ce2d8` | Not recorded | PASS (host, build) | — | Smooth applied steering and pivot curves |
| 2026-10-31 | INT-94 | `86ce2d8`+ | Not recorded | PASS (host) | [2026-10-31_INT-94/](logs/2026-10-31_INT-94/) | PASS (host, dashboard). Encoder PID diagnostics and digital IR setup |
| 2026-11-01 | INT-95 | `b0c3d1b`+ | Codex | PASS (host, build) | [2026-11-01_INT-95/](logs/2026-11-01_INT-95/) | DOUT smooth navigation. Physical commissioning pending |
| 2026-11-01 | INT-96 | `b0c3d1b`+ | Codex | PASS (host, build) | [2026-11-01_INT-96/](logs/2026-11-01_INT-96/) | Failed before fix, then PASS (host, build). Straight junction continuation. Physical retest pending |
| 2026-11-01 | INT-97 | In manifest | Nicholas | PASS (host) | — | Wheel separation and crossbar measurements |
| 2026-11-01 | INT-98 | `b0c3d1b`+ | Nicholas, Claude | PASS (host) | [2026-11-01_INT-98/](logs/2026-11-01_INT-98/) | IR spot measurements and curve steering hold. Not flashed |
| 2026-11-01 | INT-103 | image `b4dd145` | Nicholas, Claude | OBSERVED (car) | [2026-11-01_INT-103/](logs/2026-11-01_INT-103/) | First floor tracking with tuned wheels. 55 mm/s floor, two runs; run 1 lost the line at the crossing |
| 2026-11-01 | INT-104 | `b0c3d1b`+ | Nicholas, Claude | PASS (host) | At tag | Crossing heading restore. Not run |
| 2026-11-01 | INT-105 | image `23808db` | Nicholas, Claude | PASS (car) | [2026-11-01_INT-105/](logs/2026-11-01_INT-105/) | PASS at 55 mm/s. Floor run with crossing heading restore. This course section only |
| 2026-11-01 | INT-106 | `89bfc65` | Nicholas, Claude | FAIL (car) | [2026-11-01_INT-106/](logs/2026-11-01_INT-106/) | FAIL at 80 mm/s. Tracking at 80 mm/s |
| 2026-11-01 | INT-107 | `89bfc65`+ | Nicholas, Claude | FAIL (car) | [2026-11-01_INT-107/](logs/2026-11-01_INT-107/) | FAIL; PASS (host). Distance-scaled trial and PI steering. Scaling by 80/55 did not fix it |
| 2026-11-01 | INT-108 | `89bfc65`+ | Nicholas, Claude | PARTIAL (car) | [2026-11-01_INT-108/](logs/2026-11-01_INT-108/) | PASS at 55 mm/s; FAIL at 80 mm/s. PI floor runs and curve skew. Lost the line on the curve at 80 |
| 2026-11-01 | INT-109 | `89bfc65`+ | Nicholas, Claude | FAIL (car) | [2026-11-01_INT-109/](logs/2026-11-01_INT-109/) | FAIL at 80 mm/s; PASS (host). Curve speed cap. 55 mm/s runs completed |
| 2026-11-01 | INT-110 | image `42761bd` | Nicholas, Claude | FAIL (car) | [2026-11-01_INT-110/](logs/2026-11-01_INT-110/) | FAIL; PASS (host). Curve speed latch. Undershoots and goes wide on curves |
| 2026-11-01 | INT-111 | image `a81f99b` | Nicholas, Claude | FAIL (car) | [2026-11-01_INT-111/](logs/2026-11-01_INT-111/) | FAIL; PASS (host). Same speed on curves, faster integral trial. Still wide on curves |
| 2026-11-01 | INT-112 | image `c552e3f` | Nicholas, Claude | FAIL (car) | [2026-11-01_INT-112/](logs/2026-11-01_INT-112/) | Integral step 6 at 80 mm/s. Stiff on straights; curve not held |
| 2026-11-01 | INT-113 | image `7b2645a` | Nicholas, Claude | FAIL (car) | — | 60 and 65 mm/s, 100 mm/s tuning plan. Full route fails at 60 and 65 mm/s |
| 2026-11-01 | INT-114 | image `0dec93e` | Nicholas, Claude | FAIL (car) | — | 100 mm/s baseline and slew. Curve not strong enough at 100 mm/s |
| 2026-11-01 | INT-115 | image `5590232` | Nicholas, Claude | FAIL (car) | — | Bigger kick and 10 ms IR at 100 mm/s. Best so far, recovers by the end of the curve |
| 2026-11-01 | INT-116 | image `8441192` | Nicholas, Claude | FAIL (car) | [2026-11-01_INT-116/](logs/2026-11-01_INT-116/) | FAIL on curves; PASS (host). Sensors moved back, slower integral unwind. Straights and crossing pass at 100 mm/s |
| 2026-11-01 | INT-117 | image `bbbe0ce` | Nicholas, Claude | FAIL (car) | [2026-11-01_INT-117/](logs/2026-11-01_INT-117/) | Slow unwind on three curves. First curve good, second wider, third off the line |
| 2026-11-01 | INT-118 | image `04a3ee4` | Nicholas, Claude | PARTIAL (car) | [2026-11-01_INT-118/](logs/2026-11-01_INT-118/) | PASS on curves; FAIL on curve exit; PASS (host). Curves hold, curve-exit drain. 100 mm/s, these two trials |
| 2026-11-01 | INT-119 | image `cb4f942` | Nicholas, Claude | FAIL (car) | [2026-11-01_INT-119/](logs/2026-11-01_INT-119/) | FAIL; PASS (host). Curve exit swings the other way. Swings the opposite way and loses the line |
| 2026-11-01 | INT-120 | image `1ed8185` | Nicholas, Claude | FAIL (car) | [2026-11-01_INT-120/](logs/2026-11-01_INT-120/) | FAIL on S-bend; PASS (host). S-bend after the exit hold |
| 2026-11-01 | INT-121 | image `7feb06e` | Nicholas, Claude | FAIL (car) | [2026-11-01_INT-121/](logs/2026-11-01_INT-121/) | Instant exit reset hurts curves. Curves veer off, worse than the previous trial |
| 2026-11-01 | INT-122 | image `c5d9fe1` | Nicholas, Claude | FAIL (car) | [2026-11-01_INT-122/](logs/2026-11-01_INT-122/) | FAIL; PASS (host). Hold limit and hump halving |
| 2026-11-01 | INT-123 | image `be6539c` | Nicholas, Claude | PARTIAL (car) | [2026-11-01_INT-123/](logs/2026-11-01_INT-123/) | PASS on straights and curves; FAIL at one junction; PASS (host). 100 mm/s defaults from trial o. 100 mm/s; S-bend not separately checked |
| 2026-11-01 | INT-124 | `159e89b`+ | Nicholas, Claude | PASS (host) | At tag | PASS (host) with 2 known failures. Raw-input line replay tests. No physical run |
| 2026-11-02 | INT-125 | `159e89b`+ | Nicholas, Claude | PASS (host, build) | At tag | Line-trace diagnostics build |
| 2026-11-02 | INT-126 | `159e89b`+ | Nicholas, Claude | PASS (host, build) | At tag | PASS (host, build), 1 known failure. Distance-based steering law |
| 2026-11-02 | INT-127 | `159e89b`+ | Nicholas, Claude | PASS (host, build) | At tag | Line loss and reacquisition evidence. No known failures left |
| 2026-11-02 | INT-128 | `159e89b`+ | Nicholas, Claude | PASS (host, build) | At tag | Pivot junction turns |
| 2026-11-02 | INT-129 | `159e89b`+ | Nicholas, Claude | PASS (host, build) | At tag | 100 mm/s course speed guardrail |
| 2026-11-02 | INT-130 | `1161a5c`+ | Nicholas, Claude | PASS (host, build) | At tag | Subsystem rename and file merge |
| 2026-11-02 | INT-131 | `49effa4`+ | Nicholas, Claude | PASS (host, build) | — | One header per subsystem |
| 2026-11-02 | INT-132 | `79c13f1` | Nicholas, Claude | FAIL (car) | [2026-11-02_INT-132/](logs/2026-11-02_INT-132/) | Lower steering after overshoot. Veered off the track; no line trace captured |
| 2026-11-02 | INT-133 | Not recorded | Nicholas, Claude | FAIL (car) | [2026-11-02_INT-133/](logs/2026-11-02_INT-133/) | Speed stuck at half on the trial image. Stuck at 50 mm/s: hump encounter held open |
| 2026-11-02 | INT-134 | INT-133 image | Nicholas, Claude | OBSERVED (car) | — | Half speed on humps removed. Tracking good at 50 mm/s; half-speed hump request removed |
| 2026-11-02 | INT-135 | INT-134 image | Nicholas, Claude | PARTIAL (car) | [2026-11-02_INT-135/](logs/2026-11-02_INT-135/) | PASS tracking at 100 mm/s; FAIL on stops. False line loss on straights. `LINE_LOST_MM` 200 too short, set to 350 |
| 2026-11-02 | INT-136 | INT-135 image | Nicholas, Claude | PARTIAL (car) | [2026-11-02_INT-136/](logs/2026-11-02_INT-136/) | PASS on straights and curves; FAIL at junctions. Pivot ends blind past the branch. Robot did not turn to the next branch |
| 2026-11-02 | INT-137 | INT-136 image | Nicholas, Claude | PARTIAL (car) | — | PASS on those turns; FAIL on barcodes. Barcode decoder rework. Barcodes not read at all |
| 2026-11-02 | INT-138 | INT-137 image | Nicholas, Claude | FAIL (car) | — | Encoder stops and barcode visibility. Barcodes not read; robot sometimes stops |
| 2026-11-02 | INT-139b | image `27953ce` | Nicholas, Claude | PARTIAL (car) | [2026-11-02_INT-139b/](logs/2026-11-02_INT-139b/) | PASS one loop; FAIL at junction. Blind exit from a straight crossing |
| 2026-11-02 | INT-153 | image `0e93c6a` | Nicholas, Claude | PARTIAL (car) | — | PASS two barcode reads; FAIL at junction. Queued turn stopped on onset skew. Stopped with a queued turn |
| 2026-11-02 | INT-154 | `d97e3d9` | Nicholas, Claude | PARTIAL (car) | — | PASS course run; FAIL on hump. Hump not reported |
| 2026-11-02 | INT-155 | `d97e3d9`+ | Nicholas, Claude | FAIL (car) | [2026-11-02_INT-155/](logs/2026-11-02_INT-155/) | Barcode lost after a hump, hump not counted |
| 2026-11-02 | INT-156 | `d97e3d9`+ | Nicholas, Claude | PARTIAL (car) | [2026-11-02_INT-156/](logs/2026-11-02_INT-156/) | PASS barcode; FAIL on hump. Hump detector replaced with roll. All letters and a flipped print not tested |
| 2026-11-03 | INT-160 | `a7593b0` | Claude | PASS (host) | At commit | PR 149 host suite after fixing test order, a commented-out self-test and a triage script syntax error. No Arm build or Pico run; watchdog policy only |

## Audits, not tests

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-29 | INT-11 | `4b00e20`+ | Codex | OBSERVED (static) | — | See check output. Documentation validation. Markdown and static checks only |
| 2026-10-30 | INT-20 | `62ee522` | Not recorded | PASS (host, build) | — | PASS (host, build, static); plan BLOCKED. Repository findings |
| 2026-10-30 | INT-22 | `62ee522`+ | Codex | PASS (host) | — | PASS (checker). Completion record selection. Completion checker exits 1 on BLOCKED stages |
| 2026-10-31 | INT-44 | `9bcab80`+ | Codex | PASS (host, build) | [2026-10-31_INT-44/](logs/2026-10-31_INT-44/) | PASS (host, dashboard, build). Repository audit |
| 2026-10-31 | INT-45 | `2005c68`+ | Codex | PASS (host, build) | [2026-10-31_INT-45/](logs/2026-10-31_INT-45/) | PASS (host, dashboard, build). Repository audit fixes |
| 2026-10-31 | INT-46 | `5e63fe0` | Codex | PASS (host) | [2026-10-31_INT-46/](logs/2026-10-31_INT-46/) | PASS (host, dashboard). Repository audit. No source change |
| 2026-10-31 | INT-47 | `54e5ec4` | Codex | PASS (host, build) | [2026-10-31_INT-47/](logs/2026-10-31_INT-47/) | PASS (host, dashboard, build). Repository audit fixes |
| 2026-10-31 | INT-50 | `30ec622` | Codex | PASS (static) | [2026-10-31_INT-50/](logs/2026-10-31_INT-50/) | PASS (links); plan BLOCKED. Consolidated completion tracking |
| 2026-10-31 | INT-51 | `fc9fa5d` | Codex | PASS (static) | [2026-10-31_INT-51/](logs/2026-10-31_INT-51/) | PASS (links); plan BLOCKED. Retire the duplicate plan |
| 2026-11-02 | INT-159 | Not recorded | Claude | PENDING | [2026-11-02_raw-captures/](logs/2026-11-02_raw-captures/) | No run. Raw captures moved. Gzipped; `int-158-run.txt` deleted, it held no run |
| 2026-11-03 | INT-161 | `e576760` | Claude | PENDING | — | No run. Merge lineage. PR 148 merged INT-132 to INT-159 (numbering before 2026-11-09) as `e576760`; each row keeps its original source and result, including S4-U11 FAIL |
| 2026-11-03 | INT-162 | `7275347` | Claude | PASS (build) | — | Arm builds. Full course and test images with `WATCHDOG=0/1`; full course also with `TRACE=0/1`. Six images, one warning from the SDK's `mqtt.c`. Not a firmware or physical pass |
| 2026-11-03 | INT-163 | `7275347` | Claude | OBSERVED (static) | — | Cause not found. S4-U10 line 12673: LEFT event, pivot at 97.11 s, targets zero at 97.79 s, then a fresh boot; no REACQUIRE or STOPPED. Needs video or a repeat run |
| 2026-11-03 | INT-164 | `7275347` | Claude | OBSERVED (static) | — | Likely the car was lifted, not a scan fault (INT-156 lines 7806-8178: 330 mm/s against 76/124 targets, pitch -28 to +32 deg). Stop cause not telemetered |
| 2026-11-03 | INT-167 | `909bfc2` | Claude | PASS (host) | — | INT-154 showed `A->U_TURN` with no barcode event. Rule moved to `mission_barcode_event_needed` with a host test that fails when broken. Needs a car run |
| 2026-11-03 | INT-168 | `7275347` | Claude | OBSERVED (static) | — | 10 Pico restarts across INT-154 to INT-156 and S4-U10 (8 from READY, 1 STOPPED, 1 NAVIGATE). Captures cannot show whether they were manual. Open |
| 2026-11-03 | INT-169 | `7275347` | Claude | OBSERVED (static) | — | No echo never stops a run (host tests). INT-154 stops at lines 21775 and 26121 share a both-black pattern; cause of the first still unknown |
| 2026-11-03 | INT-172 | `4037b48` | Claude | OBSERVED (static) | — | Audit; no defect (source only). D14 core-0 audit, recorded in `docs/architecture.md`. Contention, lock duration and load not measured; they need the Pico |
| 2026-11-03 | INT-173 | `4037b48` | Claude | OBSERVED (build) | — | MEASURED (target compiler). `sizeof_probe.sh`: `IrTracker` 20 bytes, `LineIrSnapshot` 24 bytes; `IrRawWord` uses 3 bits. Data table updated |
| 2026-11-03 | INT-174 | `4037b48` | Claude | PASS (host, build) | — | Six Arm images. Full course `2fd47223`: text 387800, data 424, bss 84496. Host, dashboard, CLI, HTTP and field-tool tests pass |
| 2026-11-03 | INT-175 | `02d30fa` | Nicholas, Claude | OBSERVED (car) | — | User report. INT-168 restarts: Nicholas reports the battery was not connected properly, a hardware fix. Closed; no firmware cause investigated |
| 2026-11-03 | INT-177 | `ebb3a2c` | Claude | OBSERVED (build) | — | Static app data 2076 B; `Mission` 212, `bypass_control_t` 72, `MotionController` 60 B. `TRACE=0` saves 464 B bss |
| 2026-11-03 | INT-178 | `c2b0915` | Claude | PASS (host, build) | — | Bit cuts. Full course `dbf8acdf`: text 388856, bss 84316 (was 387800, 84496). `Mission` 212 to 164 B, `MotionController` 60 to 44, telemetry 23 to 22 B |
| 2026-11-03 | INT-179 | `3c10232` | Codex | PASS (host) | — | Full suite, stall guard on and off, 5 M snapshot reads, 300000 frame stress, 100000 mapping exchanges |
| 2026-11-03 | INT-180 | `3c10232` | Codex | PASS (build) | [profile measurements](logs/2026-11-03_INT-180-build-matrix.json) | PASS (Arm build only). `nic2_wd0_trace0_test1_course0`; text/data/bss 385132/424/84044 bytes; image hash in measurements; not flashed |
| 2026-11-03 | INT-181 | `3c10232` | Codex | PASS (build) | [profile measurements](logs/2026-11-03_INT-180-build-matrix.json) | PASS (Arm build only). `nic2_wd0_trace1_test1_course0`; text/data/bss 387492/424/84508 bytes; image hash in measurements; not flashed |
| 2026-11-03 | INT-182 | `3c10232` | Codex | PASS (build) | [profile measurements](logs/2026-11-03_INT-180-build-matrix.json) | PASS (Arm build only). `nic2_wd1_trace0_test1_course0`; text/data/bss 385616/424/84064 bytes; image hash in measurements; not flashed |
| 2026-11-03 | INT-183 | `3c10232` | Codex | PASS (build) | [profile measurements](logs/2026-11-03_INT-180-build-matrix.json) | PASS (Arm build only). `nic2_wd1_trace1_test1_course0`; text/data/bss 387980/424/84528 bytes; image hash in measurements; not flashed |
| 2026-11-03 | INT-184 | `3c10232` | Codex | PASS (build) | [profile measurements](logs/2026-11-03_INT-180-build-matrix.json) | PASS (Arm build only). `nic2_wd0_trace1_test0_course1`; text/data/bss 387764/424/84496 bytes; image hash in measurements; not flashed |
| 2026-11-03 | INT-185 | `a5157f9` | Codex | PASS (host) | — | Eight field-tool cases: speed gaps keep transitions, malformed traces rejected, unique IDs |
| 2026-11-03 | INT-186 | `a5157f9` | Codex | OBSERVED (static) | [capture review](logs/2026-11-03_INT-186-capture-review.md) | AUDIT (no robot run). Acquisition-time speeds compared with arrival-time estimates; physical root causes not inferred |
| 2026-11-04 | INT-187 | `7275347`+ | Claude | PASS (host, build) | At commit | Crossing exit steering. INT-154/156 traces: steer 90 on 23 of 24 straight exits, heading swing 7-9 deg. Image `e10c2767`; jerk fix not yet shown |
| 2026-11-04 | INT-188 | `e10c276` | Nicholas, Claude | FAIL (car) | [2026-11-04_INT-188/](logs/2026-11-04_INT-188/) | FAIL at junctions. Veered left at straight crossings. Both black under 9 mm, so no commit and the INT-187 fix never ran; left-only steer 60 turned about 20 degrees at t=25960 |
| 2026-11-04 | INT-189 | `7275347`+ | Claude | PASS (host, build) | At commit | Straight crossing uses the 36 mm ramped advance, candidate travel 6 mm. Image `770123ac`. A false commit on a curve is not ruled out |
| 2026-11-04 | INT-190 | `770123a` | Nicholas, Claude | PARTIAL (car) | [2026-11-04_INT-190/](logs/2026-11-04_INT-190/) | Two crossings committed, then veered left: first touch about 40 mm past the bend limit, heading -11 to -36 deg. Left IR stayed black 27-29 mm |
| 2026-11-04 | INT-191 | `7275347`+ | Claude | PASS (host, build) | At commit | First touch after a crossing is a bend only past 100 mm. Image not flashed; no physical run |
| 2026-11-04 | INT-193 | `cf1a3c2` | Claude | PASS (host, build) | — | All unmerged branches together. Seven Arm images plus `TERRAIN_DIAGNOSTICS=1`. Full course `76e62048`: text 388412, bss 84272; `Mission` 160 B |
| 2026-11-04 | INT-194 | `6839d26` | Claude | PASS (host, build) | — | Course speed 100 mm/s, `full_course` at `TRACE=0`, 200 ms U-turn pause. Host passes at 100, 80, 60 mm/s. `TRACE=0` saves 464 B RAM (bss 83808 vs 84272) |
| 2026-11-04 | INT-196 | `2ee4dc2` | Claude | PASS (host, build) | — | Slowing starts 183 mm out to rest at 150 mm; inside 150 mm brakes at once. Host passes at 100, 80, 60 mm/s. Full course `f0376ee7`: text 386996 |
| 2026-11-04 | INT-197 | `9207d79` | Claude | PASS (host, build) | — | Straight crossings roll through at course speed, 36 mm without steering. Host passes at 100, 80, 60 mm/s. Full course `30c7ecfd`: text 387092 |
| 2026-11-05 | INT-199 | `3fd95ba` | Claude | PASS (host, build) | — | Distance-based PI steering, lower rates. Four deliberate law faults each failed the host tests |
| 2026-11-05 | INT-200 | `3fd95ba` image | Nicholas | OBSERVED (car) | — | Nicholas reports line tracking very successful. Monitor log empty: no telemetry or lap count |
| 2026-11-05 | INT-198 | `3fd95ba`+ | Nicholas | OBSERVED (car) | Capture not kept | Bypass trials. Rested 139-148 mm from the box after slowing from 193 mm. Depth read 126, 131, 230 mm; side look losing the box suspected |
| 2026-11-05 | INT-202 | `a3d6741` | Claude | PASS (host, build) | — | Right-angle bypass, step-out, 20 mm rear gap, slow stop, ghost filter. Host at 100 mm/s; 60 mm/s last run before the final edits |
| 2026-11-05 | INT-203 | `a3d6741` image | Nicholas | OBSERVED (car) | — | Nicholas reports four good bypass runs in a row; no log. An earlier run took the farther-edge side, cause unknown |
| 2026-11-05 | INT-204 | `b24356b` | Nicholas | OBSERVED (car) | Capture not kept | Nicholas reports two full-course runs good except the hump. Log shows four bypasses, five crossings, no barcode reads, no humps |
| 2026-11-05 | INT-205 | `3528048` | Claude | PASS (host, build) | — | CI checks, IMU graphs (simulated data), clearance reporting. BARR-C 0 findings, deliverables 0 problems; eight Arm profiles build |
| 2026-11-05 | INT-206 | `eb8b2e9` | Claude | PASS (host, build) | — | Watchdog on for `FULL_COURSE=1`, lap scorer removed. Watchdog has not run on a Pico |
| 2026-11-05 | INT-208 | `560fb34` image | Nicholas, Claude | OBSERVED (car) | Capture not kept | First bypass turn undershoots, pivot back overshoots: two pivots ended at 118 and 119 deg against the 120 deg limit. One bypass stopped 79% through the final turn, reason unknown |
| 2026-11-05 | INT-211 | `d6914ba`+ | Claude | PASS (host, build) | — | Bypass side from the shorter box-edge distance only (`range * tan(angle)`); clearance no longer rejects a side. Trial profile builds |
| 2026-11-05 | INT-212 | `d6914ba` image | Nicholas, Claude | OBSERVED (car) | Capture not kept | One run stopped after scanning (width 178 mm, no faults, no log), cause unknown. Rerun bypassed. Two sweeps took 15.5 s from stop to first pivot |
| 2026-11-05 | INT-213 | `560fb34`+ | Claude | PASS (host, build) | — | Shorter-edge rule on `7e8566e` and `560fb34`, watchdog removed. `terrain.c` helpers rewritten in plain C (undeclared math calls). Six images build |
| 2026-11-05 | INT-214 | `560fb34`+ | Nicholas, Claude | OBSERVED (car) | Capture not kept | Two bypasses, both left. First: width 201 mm, leg out 206 mm, so edges about 81 mm left and 120 mm right: left as the rule says. Side look got no echo |
| 2026-11-05 | INT-215 | `560fb34`+ | Nicholas, Claude | OBSERVED (car) | Capture not kept | Box offset left (amount not recorded), car still passed left with too little clearance. Leg out 204 mm implies edges 79 mm left, 115 mm right. Stepped out 40 mm mid-pass |
| 2026-11-07 | INT-216 | `84e5bf5` | Codex | PASS (build) | [baseline measurements](logs/2026-11-07_INT-222/layout.json) | Clean-source DWARF and sizeof baseline, Arm GNU 13.3.1. Course TRACE=0: text 387912, data 424, bss 83836 |
| 2026-11-07 | INT-217 | `84e5bf5`+ | Codex | PARTIAL (build) | — | Six struct sizes reduced but changed-line BARR found 9 findings. Superseded by INT-222 |
| 2026-11-07 | INT-218 | `84e5bf5`+ | Codex | PASS (host) | — | First existing host regression suite passed. Intermediate layout source not retained; this is not acceptance evidence for the final tree; rerun in INT-222 |
| 2026-11-07 | INT-219 | `84e5bf5`+ | Codex | PASS (static) | — | PASS (style). Kept existing signed bit-field declarations and type names, split moved multi-variable declarations; changed-line BARR has zero findings. Rechecked in INT-222 |
| 2026-11-07 | INT-220 | `84e5bf5`+ | Codex | FAIL (build) | — | Terrain probe failed on the deleted `TerrainTravelQuality`; obsolete probe line removed |
| 2026-11-07 | INT-221 | `84e5bf5`+ | Codex | PASS (build) | [sizes and source hashes](logs/2026-11-07_INT-222/layout.json) | All six Arm sizeof probes pass after removing the obsolete Terrain type and `sizeof` on bit-fields |
| 2026-11-07 | INT-222 | `84e5bf5`+ | Codex | PASS (host, build) | [51-type inventory](logs/2026-11-07_INT-222/struct-layout.md) | Field reordering. Course TRACE=0: text 388140, bss 83832 (4 B saved). `Mission` 160 to 144, `MissionInputs` 104 to 96 B |
| 2026-11-08 | INT-223 | `87f4eb8` | Nicholas, Claude | PASS (host, build) | [evidence](../../docs/team.md#7-correctness-and-performance-evidence) | Minimum-bit narrowing, against `84e5bf5`. Course TRACE=0: text 387912 to 390356 B, bss 83836 to 83728 B; `MissionInputs` 104 to 64 B. Timing is INT-224/225 |
| 2026-11-09 | INT-224 | `017f5ed` trial | Nicholas, Claude | OBSERVED (car) | [usb.log](logs/2026-11-09_INT-224/usb.log), [monitor](logs/2026-11-09_INT-224/monitor.log.gz) | Timing before narrowing. Max body: Motion 889 us, Mission 3716 us (bypass), I/O 2302 us. Stack peak 672/1400/480 of 2176 B. Motion ovr 0; Mission and I/O ovr +50/s in every segment, idle included. Monitor is from a first attempt without USB capture |
| 2026-11-09 | INT-225 | `0727821` trial | Nicholas, Claude | PARTIAL (car) | [usb.log](logs/2026-11-09_INT-225/usb.log) | Timing after narrowing, one boot, two bypasses. Max body: Motion 1275 us, Mission 5229 us (scan, bypass start), I/O 2400 us; stack peak 664/1256/544 of 2176 B; ovr as INT-224. Bypass 1: stall guard stopped the first 36 mm/s pivot. Bypass 2: no stall, pivot back came early (observed by Nicholas). Console from terminal scrollback; no monitor capture |
| 2026-11-09 | INT-226 | `b0847bd` trial | Nicholas, Claude | OBSERVED (car) | [usb.log](logs/2026-11-09_INT-226/usb.log) | Bypass with `[BYPASS]` trace; pivot back early (Nicholas). Side ray never echoed off the box (flank 0, side 8191), so rear = box front 221 + `BYPASS_MIN_DEPTH_MM` 200 + 95 = 516 mm, where IN began |
| 2026-11-09 | INT-227 | `b0847bd` trace format | Nicholas, Claude | OBSERVED (car) | [usb.log](logs/2026-11-09_INT-227/usb.log) | Bypass on the 210 x 70 mm test object. Line seen at the end of RETURN, then NAVIGATE and STOPPED with no fault, before the 120 mm advance (Nicholas). Trace lacks the `8429d8c` lines, so the image that ran is unclear; side ray saw the box (flank 93) |
| 2026-11-09 | INT-228 | `8429d8c` trial | Nicholas, Claude | FAIL (car) | [usb.log](logs/2026-11-09_INT-228/usb.log) | Bypass on the test object. Left IR read black (ir=10) as the IN pivot ended, 223 mm left of the line, so RETURN ended after 20 ms; the 120 mm advance ran (front 1533 to 1414 mm), the left pivot found no line and Mission stopped (phase 2, motion DONE) |
| 2026-11-09 | INT-229 | `be17ef6` trial | Nicholas, Claude | PASS (car) | [usb.log](logs/2026-11-09_INT-229/usb.log) | Bypass rejoin on the test object. RETURN began on black again (ir=10) and was ignored; DONE on black after white at left 94 mm, then advance, left pivot, brake and FOLLOW_LINE. IR also read black beside the line during PARALLEL to REAR |
| 2026-11-09 | INT-234 | `7da3c66`+ `[BCPIN]` diag, BARCODE_DIAGNOSTICS=1 | Nicholas, Claude | PARTIAL (car) | [monitor.log.gz](logs/2026-11-09_INT-234/monitor.log.gz) | Half Z and full A read, full F did not. Only the A EVENT BARCODE arrived; Z read (telemetry Z->RIGHT) but telemetry #64 to #68 and its event were lost in a 3 s gap: mqtt_utk.c dropped any frame the send buffer refused. BARS traces also have gaps, so F widths are not recoverable |
| 2026-11-09 | INT-238 | `7da3c66`+ diag, MQTT frame retry, cut 10, barcode clear after navigation, BARCODE_DIAGNOSTICS=1 | Nicholas | PASS (car) | Capture not kept | Nicholas reports a successful run: barcodes read, dashboard card shows the letter and clears it after the barcode turn |

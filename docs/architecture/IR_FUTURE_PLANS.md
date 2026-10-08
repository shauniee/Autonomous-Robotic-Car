# Subsystem 3 IR future plans

## 1. Baseline and boundaries

Owner: Nicholas / 2501552. Active baseline: [proportional line following](LINE_FOLLOWING.md), paired RAM calibration, explicit <=100 ms last-direction recovery and unchanged [S2 interfaces](../../TEAM.md). This file records future work, not implemented capability or passed tests. [Barcode](BARCODE.md) retains source geometry and sample observations.

Line steering is S3's outer guidance loop. Future per-wheel encoder speed/PID control belongs to Mahesha/S2; this plan does not modify it. Full Mission/Safety integration remains the proposed [system design](../ARCHITECTURE.md).

## 2. Establish a measured P baseline

Record mounted sensor height/spacing, line width, calibrated ranges, lighting, exact source/image, battery setup, duties, lateral error, overshoot, losses and stop measurements. Test straight/curve tracks and several repeats at a low duty before raising speed. Confirm that both-white actually represents loss for the mounted geometry; two sensors alone do not prove centring or the identity of the original line.

Tune P against measured error and repeatability. Check sensor calibration and motor asymmetry before adding control complexity. Use the same course, setup and acceptance measures when comparing controllers. Define numerical targets before claiming an improvement.

## 3. Add control features only after evidence

| Addition | Motivation and implementation guard | Evidence needed |
| --- | --- | --- |
| Filtered PD | Reduce measured oscillation/overshoot. Use measured acquisition intervals, finite inputs and bounded derivative filtering; suppress first D after start/recovery/calibration. | Better repeated curve/error results than P without extra loss or excessive output noise |
| Integral term | Correct persistent bias only after calibration/mechanical causes are checked. Bound integral, prevent saturation windup, and reset/freeze it during stop/loss/capture/broad black. | Steady-error reduction without increased overshoot or prolonged saturation |
| Sensor filtering | Reduce measured reflectance noise; preserve original acquisition time and bound latency. | Recorded noise and response delay under changing lighting/edges |
| Duty ramps / adaptive speed | Reduce abrupt requests or slow near larger errors. S2 remains sole writer; stop must bypass ramps. | Measured stopping and tracking, not inferred speed |
| Timing diagnostics | Record releases, processing time, sample age, misses and stack margin. No backlog replay. | Timing under realistic load; measured stop output and watchdog behaviour |
| Better ambiguity handling | Validate mounting and persistently white/black observations using history and mission context. Do not guess a junction or finish. | Known course poses, crossings and original-line recovery cases |
| Sensor fault diagnostics | Test disconnected/stuck sensors using justified plausibility/progress or hardware evidence. In-range ADC alone is insufficient. | Known injected faults and false-positive rates |

Retain bounded correction/duty, independent stop, command expiry and freshness under every controller. New gains, filters and recovery bounds require new measurements and test cases; do not relabel the old PID tests as new P/PD/PID evidence.

## 4. Barcode capture and decoder stages

GP16 DOUT is reserved for S3; barcode AOUT remains disconnected. Before dependent code, confirm installed comparator polarity, threshold, mounting height/lateral offset, traversal direction, symbology, framing/quiet zones, narrow/wide tolerance, printed scale and command mapping. A/Z samples do not establish the mission command table.

| Future state | Work / exit |
| --- | --- |
| Disabled / unconfigured | No navigation output; remain here until required course/configuration facts are confirmed |
| Wait quiet zone | Confirm the configured frame boundary before capture |
| Capture frame | Bounded edge ISR stores ordered transitions, acquisition times and coherent motion context; task derives both bar and space widths |
| Validate frame | Check complete framing, ordered widths, orientation and known symbol; reject partial/malformed/unknown/time-out reads |
| Queue event | Copy one mapped event with ID, symbol/action and acquisition time into the proposed bounded FIFO |
| Wait for rearm | Prevent duplicate events from one encounter; rearm on the configured quiet-zone/encounter boundary |
| Fault | Capture/event overflow or critical acquisition failure; independent fault notification, no guessed command |

DOUT HIGH/LOW is not assumed to equal black/white until measured. Widths based only on time vary with speed; use coherent encoder-distance snapshots or a validated compensation method. Encoder feedback is currently absent, so compensation and navigation integration remain deferred.

Keep edge capture independent of the 20 ms analogue loop. ISR work is bounded and uses only interrupt-safe kernel operations; decoding runs in task context. Measure edge rate before choosing fixed capture/FIFO capacity. Use copied message-buffer events or explicitly managed mailbox storage; a pointer to a temporary/stack frame is not a persistent event.

## 5. Mission integration and future test cases

Line observations remain latest-value snapshots. Barcode events remain a separate once-per-encounter FIFO; capture/queue overflow must be visible and fault explicitly. Mission decides whether an event is usable in the current route context and requests an ID-bound S2 action. Barcode decoding never writes motors or clears stop. Reject/purge stale context on reset and interrupted actions. Enable turns only with confirmed mapping, encoder completion, deadline and reacquisition guards.

| Case | Scenario | Required evidence |
| --- | --- | --- |
| BC-P01 | A/Z at both supplied scales | Ordered bars and spaces, complete frame and correct symbol; source symbology confirmed first |
| BC-P02 | Partial, malformed, unknown and reverse frames | Explicit rejection or supported validated orientation; no guessed command |
| BC-P03 | Speed variation | Correct widths using measured distance/validated compensation |
| BC-P04 | Repeated encounter / quiet-zone rearm | Exactly one event per encounter; later encounter accepted |
| BC-P05 | Capture/FIFO overflow and stale context | Explicit fault/rejection; no lost event represented as success |
| BC-P06 | Concurrent ADC, button stop and barcode bursts | Bounded sampling/stop timing and no peripheral ownership conflict |
| BC-P07 | Four mapped actions and recovery | Encoder completion, original-line guard and measured accuracy |

These are planning labels, not official test IDs or run results. Allocate new permanent S3/INT/SYS IDs in testing/ before implementation/trials. No barcode decoder, navigation FIFO or new task is created by the proportional refactor.

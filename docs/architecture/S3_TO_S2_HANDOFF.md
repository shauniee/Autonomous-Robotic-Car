# S3 → S2 requirements handoff

From: Nicholas — S3 (Line Tracking / Barcode)  
To: Mahesha — S2 (Motion)  
Prepared: 2026-10-13

Mahesha, S3 needs Motion to execute line-following demands safely now and to
provide measured encoder feedback and repeatable actions for future barcode
navigation. Please review the existing interface and the deliverables below.
This is a documentation handoff; it does not implement or approve S2 changes.

## 1. Ownership and current status

| Owner | Responsibility |
| --- | --- |
| S3 / Nicholas | Paired IR acquisition, calibration, line classification and proportional guidance; future barcode capture, decoding and navigation events |
| Application / Mission coordination | Convert observations into movement requests; own start/reset, recovery, event interpretation and command handover |
| S2 / Mahesha | Exclusive motor-output writer; request validation, stop/reset and service health; future encoders, wheel-speed PID and motion actions |
| Safety / team | System-wide stop/fault policy and agreed physical acceptance limits |

Use [Line Tracking](LINE_TRACKING.md), [Barcode](BARCODE.md),
[Motion](MOTION.md) and the [shared contracts](../ARCHITECTURE.md#shared-contracts)
as the owning specifications. S3 does not write motor PWM. Proportional line
guidance and S2's planned per-wheel speed PID solve separate problems.

**Checkout verification:** the S3 snapshot reader and S2 duty/stop/health API are
present. The edited [IR-to-Motion guide](IR_MOTION_HANDOFF.md) describes an
implemented application controller, but this checkout has no `line_control`,
`task_line_follow` or `line_follow_start` source/startup call. Its referenced
INT-11–INT-14 fixtures are also absent from the current integration log and test
sources. Therefore the controller behavior in section 3 is documented policy,
not a source-verified running integration here. Resolve this source/document
mismatch before treating the bench as ready for physical trials.

Barcode capture/decoding/navigation and encoder actions remain proposed in this
checkout. No successful physical tracking, stopping or navigation is claimed.

## 2. Existing S2 interface to preserve

The declarations are in [subsystem2.h](../../app_program/subsystem2/subsystem2.h)
and [motion_policy.h](../../app_program/subsystem2/motion_policy.h). The
[current bench contract](IR_MOTION_HANDOFF.md#current-interface-contract) owns
the integration behavior; this table summarizes it for review.

| Interface | Requirement for the consumer / meaning |
| --- | --- |
| `MotionRequest.left`, `.right` | Finite forward PWM duty fractions, inclusive 0…0.35. These are not measured wheel speeds; the current API cannot request reverse. |
| `.sampled_ms` | Preserve the original paired IR acquisition time in the monotonic millisecond clock. Unsigned age ≤60 ms is accepted; >60 ms expires. Republishing a cached sample must not refresh its timestamp. |
| `.enabled` | Authorize movement only with usable sensing and an active movement state. False requests braking without clearing the persistent stop latch. |
| `s2_publish(request)` | Bounded latest-value copy, not a FIFO. Check the returned `ER`; failure requires local inhibition and the independent stop path. Copy success does not prove output acceptance or movement. |
| `s2_stop()` | Persistent stop plus independent Motion wake-up, without acquiring the request mutex. S2 boots stopped. Every stop invalidates the previous brake acknowledgement; check the return value. |
| `s2_reset_stop()` | Requires the latest stop's successful brake service. `E_OBJ` or any other failure keeps the consumer stopped. Success clears cached requests and invalidates previously copied work; reset itself never commands movement. |
| `s2_health(&health)` | Consume fields only after a successful read. `sequence` and `serviced_ms` indicate successful output service; `stopped` and `brake_acknowledged` indicate software inhibition and brake service, not measured stationarity. |

Use task context after initialization. Runtime stop/reset callers must share
Motion's core 1; cross-core stop/reset is unsupported. Do not acquire the API's
mutexes from an ISR. Motion normally services every 20 ms and can wake for stop;
this cadence is not a measured physical stop-latency bound. TEST firmware always
inhibits motor movement.

Keep one authorized request producer. Future Mission/Obstacle handover needs
agreement before adding another producer. S2 must remain independent of IR
types, barcode interpretation and network reporting.

## 3. Line-following behavior S2 needs to support

The [IR-to-Motion guide](IR_MOTION_HANDOFF.md#tracking-and-recovery) owns these
bench choices. They require the controller source reconciliation noted above.

- With fresh calibrated VISIBLE guidance, use normalized steering in −1…+1:
  `correction = 0.10 * steering`,
  `left = clamp(0.10 + correction, 0, 0.35)` and
  `right = clamp(0.10 - correction, 0, 0.35)`. Negative steering requests a left
  turn; positive requests a right turn. Confirm that wiring realizes this sign.
- BROAD_BLACK while following requests 0.05 on each wheel. It does not establish
  a junction or finish. Equal duties do not establish equal speed or centering.
- LOST uses the last nonzero steering direction: one wheel at 0.10, the other
  zero. No known direction means stop. Recovery has an absolute 500 ms deadline
  from entry, never renewed by samples. Two distinct consecutive VISIBLE
  acquisitions before that deadline permit resumption; LOST/BROAD_BLACK resets
  the streak. This does not prove reacquisition of the original course line.
- Acquisition age >60 ms, invalid acquisition/calibration, active capture,
  unusable visible guidance, failed reads/publication or expired service health
  require inhibition and stop. Retain the original sample time in every request.
- GP20 completes calibration before start can be accepted. GP21 stop dominates
  simultaneous start and is handled before snapshot/health reads. After a stop,
  release GP21 and clear the cause; a fresh GP20 press resets only after brake
  acknowledgement, and a separate release/press starts. Held or bouncing buttons
  must not replay a start. Valid calibration is retained after stop.

### Bench review and physical checks

- [ ] Reconcile the documented controller with the source and startup wiring;
      review the producer boundary together before motor trials.
- [ ] Confirm left/right motor identity, forward polarity and steering sign with
      raised wheels under the [power procedure](../GETTING_STARTED.md#power-and-flashing).
- [ ] Verify boot brake, GP21 stop, latest-stop acknowledgement and separate
      reset/restart; measure electrical brake timing and physical stopping separately.
- [ ] Measure unequal wheel response at equal duties; record motor asymmetry
      before proposing compensation or tuning changes.
- [ ] Record straight/curve, broad-black, controlled-loss and recovery-timeout
      trials with mounted sensor calibration, lighting, surface and power details.

The listed duties and recovery deadline are bench choices, not calibrated speed,
accuracy or stopping limits.

## 4. Future S2 capabilities needed for barcode navigation

These are requirements for owner review, not implemented API signatures.
[Barcode](BARCODE.md#business-logic) owns decoding/event behavior and
[Motion](MOTION.md#business-logic) owns encoder/PID/action behavior.

| Capability | S3 / Mission dependency |
| --- | --- |
| Coherent encoder feedback | Left/right counts, direction, acquisition time, sequence and validity, plus measured speeds/distances with explicit units. Barcode width processing needs encoder travel or separately validated speed compensation. |
| Capture-compatible encoder context | Agree a bounded, ISR-safe way to associate barcode edges with coherent encoder context. The barcode ISR must not use blocking task APIs, decode frames or allocate memory. |
| Measured wheel-speed control | Independent per-wheel PID and straight-travel correction; calibration, saturation and encoder/stall failures are handled by S2. Line P guidance remains separate. |
| Four navigation actions | Support Mission-selected left, right, straight and U-turn actions through calibrated distance/angle targets. Mission interprets S3's mapped event; S3 never executes the turn directly. |
| Action lifetime | Unique action ID, leased command, absolute action deadline, active/done/failed status and failure reason. Renewing the lease must not restart or duplicate the action. Retain completion until acknowledged. |
| Cancellation and safe inhibition | Stop, expiry, fault, stall or obstacle preemption cancels/inhibits the action. Reset invalidates old commands; a new authorized command is required afterward. |
| Measured readiness/stationarity | Report fresh measured feedback where stationary operations need it. Existing brake acknowledgement and service sequence are insufficient. |

Mission coordinates navigation completion and bounded original-line reacquisition;
S2 reports execution status. Barcode events outside the permitted mission context
must not redirect movement. Do not guess symbol mappings, turn geometry, reverse
permission, tolerances, lease durations or stall thresholds.

## 5. Requested S2 deliverables and acceptance evidence

- [ ] Review and confirm the existing request/stop/reset/health contract with Nicholas.
- [ ] Record motor/encoder logic levels, decoded counts per revolution, polarity,
      wheel diameter and track width in the owning guides; resolve D03.
- [ ] Propose the encoder feedback and motion-action interfaces against the shared
      contracts, including bounded capture access and ownership, before implementation.
- [ ] Supply measured wheel control and ID-bound action execution with completion,
      failure, cancellation and retained acknowledgement behavior.
- [ ] Agree and record PID/accuracy, stall, lease/deadline and stationarity acceptance
      limits through D07, D08 and D10. Coordinate barcode mapping/course dependencies
      through D01/D02 rather than choosing them inside S2.
- [ ] Record exact source/image/profile, firmware results and physical measurements
      in the existing test records, with saved evidence and remaining limitations.

All unresolved values belong in the single
[pending-decisions table](../PROJECT_SPECS.md#pending-decisions). This handoff
does not create a second decision register or a new deadline.

| Validation level | Cases / recording location | What it establishes |
| --- | --- | --- |
| Build / static review | [Build profiles](../GETTING_STARTED.md#build-profiles), [S2 log](../../testing/subsystem2/TEST_LOG.md), [integration log](../../testing/integration/TEST_LOG.md) | Compilation and interface consistency only; run shared build profiles sequentially. |
| Firmware self-check | Existing S2-U03; record actual on-target TEST output in the S2 log | Executed Motion policy assertions with motor outputs inhibited. A compiled image is not a pass. |
| Physical bench | Planned T02, T03, T11 and T15 in the [testing procedure](../../testing/README.md#planned-integrated-trials); record applicable integration/system cases | Measured boot/stop/reset, tracking, loss recovery and response under load. |
| Future navigation | Planned T04, T05 and T14; [system log](../../testing/system/TEST_LOG.md) | Barcode traversal, four encoder actions, repeated accuracy, cancellation, expiry and faults. |

INT-03/05 and S2-U04/U05 describe historical fake-kernel checks; their retired
runners are not current execution instructions. Do not claim INT-11–INT-14 as
present or passed until their definitions/fixtures are available. Allocate new
permanent IDs in the owning logs before executing additional proposed cases;
never reuse retired IDs.

Follow the [evidence procedure](../../testing/README.md#run-and-record): define
steps and acceptance measurements before a run, save actual evidence and append
results without overwriting history. Builds, console state transitions and
software brake service do not establish physical wheel movement or stationarity.

The phase-specific [older motion requirements](MOTION_INTEGRATION_REQUIREMENTS.md)
retain historical context. Where they describe a missing producer or proposed
button/recovery policy, reconcile against current source and the owning bench
guide rather than assuming either document proves implementation.

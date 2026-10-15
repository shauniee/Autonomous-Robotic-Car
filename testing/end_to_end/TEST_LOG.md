# End-to-end mission tests

These are physical acceptance scenarios on the normal, motor-enabled mission image. No case has been executed or accepted. A build, an inhibited firmware fixture, source review, or dashboard simulation is not an end-to-end result.

## Common preconditions and evidence

Before any powered movement, resolve the electrical release, installed sensor compatibility, course rules, and numerical pass limits in [PROJECT_SPECS](../../docs/donotsubmit/PROJECT_SPECS.md#pending-decisions). Confirm motor/driver/supply ratings, Pico-safe sensor levels, barcode format, junction and finish conventions, obstacle bounds, and stopping requirements. Record the agreed numeric limits and their approving source here before execution. Until then, every E2E case remains **PENDING**; do not substitute guessed timing, distance, accuracy, clearance, or height thresholds.

Use the exact confirmed course and a normal `TEST=0`, `IR_ONLY_TEST=0` mission image. Record source revision, build profile, UF2 SHA-256, robot identity, date, tester, wiring, battery/power, broker address, sensor/calibration state, configuration revision, mapping request ID, run epoch, and route layout. Keep the firmware image identical whenever a case says the mapping changes without a rebuild. Follow the single-power-source and calibration sequence in [GUIDE](../../GUIDE.md); use battery power for normal movement and disconnect USB before applying battery power.

Start one timestamped status/MQTT capture and one continuous video capture before the run; include the console capture where the approved setup provides it. Align clocks or record the offset. For each event, retain raw evidence for barcode symbol/action, action ID and completion, encoder reference/observed distance and angle, IR acquisition sequence/time, terrain/hump status/history, scan profile, mission state/reason, stop/brake acknowledgement, mapping revision, and elapsed time. Append failures and reruns; never overwrite them. Compare measurements only with the limits resolved above.

## E2E01 — Complete course with runtime mapping

**Status: PENDING.** This is one complete mission from boot through confirmed finish. Do not start if any common precondition, electrical limit, course rule, or acceptance threshold remains unresolved.

### Procedure

1. Confirm the approved course is clear and marked. Install the frozen normal image. Record its source revision, complete build profile, and UF2 SHA-256. Start the broker and evidence captures, then boot on the approved battery supply with GP20 and GP21 released. Verify the robot remains braked and reports no active run.
2. Perform combined black/IMU calibration exactly as in the GUIDE: place both line sensors over the black reference at their installed height, press/release GP20 once, hold the robot level and still, and wait for the one-beep completion. Confirm telemetry reports successful black and IMU calibration. Move both sensors to the white reference, press/release GP20 separately, and wait for the two-beep completion. Confirm accepted contrast and fresh line guidance. Any timeout, failure beep, stale sensor, or movement invalidates this attempt; resolve the cause and restart the required calibration sequence.
3. Prepare a complete four-letter mapping with one distinct letter assigned to each of left, right, straight, and U-turn; leave all other letters unmapped. Validate the file locally, send it through the documented mapping helper while stopped, and retain the request and acknowledgement. Require a matching request ID, successful applied result, and applied revision. Confirm the UF2 hash is unchanged and no build/reflash occurred. Do not infer that editing the file installed the map.
4. Wait for fresh `READY` telemetry showing the required calibration, settings, sensors, and mapping revision. Position the robot at the approved start line, clear the movement area, release both buttons, then press/release GP20 as a separate local start. Require a fresh run epoch and `FOLLOW_LINE` before the robot advances.
5. Traverse the confirmed course and present the four assigned markers in the approved sequence. For each marker, retain the decoded symbol/orientation, mapped action, run epoch, junction qualification, generated Motion action ID, encoder progress, matching completion acknowledgement, and next mission transition. Verify each of the four actions executes exactly once at its intended junction and that both wheel targets meet the approved completion rule. Record measured distance and angle error against the agreed limits.
6. Traverse each approved reference hump, including the required one-wheel reference if the confirmed route includes it. Save the terrain event, raw/reference measurement, accepted height, current-run count, last height, and maximum height. Continue to the approved static obstacle. Verify fresh forward detection, stopped/brake acknowledgement before scan, the selected permitted bypass, and reacquisition of the original line using fresh line/pose/heading context. Record the actual route and clearance references against the approved bounds.
7. Continue to the confirmed finish marker after the agreed action count and minimum travel. Verify the configured finish condition, `COMPLETE` state, and acknowledged brake. Record total elapsed time, route/markers visited, action/hump counts, last/max height, mapping revision, and all final status/event snapshots.

### Pass oracle

Pass only when the same unchanged firmware image boots, calibrates, accepts the runtime four-action map, starts only from the separate local GP20 press, completes all four distinct actions at qualified junctions, records the reference hump evidence, safely bypasses and reacquires the original line around the approved static obstacle, and reaches the confirmed finish braked. Physical route, event history, action IDs, terrain peak, and telemetry must agree with the observed run and meet every previously agreed accuracy, clearance, stopping, and time limit. Any unresolved threshold, missing reference, unexpected action, stale evidence, intervention, non-fresh reacquisition, or unconfirmed finish leaves the case PENDING or FAILED and must be recorded.

## Results

| Date | Case | Revision / UF2 SHA-256 | Power, wiring, calibration and mapping | Tester | Result | Measurements / elapsed time | Evidence |
| --- | --- | --- | --- | --- | --- | --- | --- |
| | E2E01 | | | | PENDING | Numeric/course acceptance limits unresolved; no physical run recorded | |

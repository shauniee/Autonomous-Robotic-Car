# IR-to-Motion bench integration

Owner: Nicholas / application coordination, using Mahesha's existing S2 interface. This guide owns the implemented bench movement policy. [Line Tracking](LINE_TRACKING.md) owns sensing/calibration; [Motion](MOTION.md) owns actuation; the realignment tracking issue (#62) owns implementation/validation status. Full Mission/Safety remains proposed.

## Running path

`app_main` initializes every subsystem before starting tasks. After S3 starts it calls `line_follow_start()`. In TEST builds, integration fixtures run after the existing test gate and subsystem checks; S3 and the controller start afterward. S2 always inhibits motor output in TEST builds.

The live path is S3 acquisition → coherent `S3IrSnapshot` → application `line_control`/`task_line_follow` → `s2_publish(MotionRequest)` → S2 Motion task → motor driver. The controller runs on core 1 at priority 14 every 10 ms, below S3 priority 12 and Motion priority 13. It adds one task/standard application stack, with no new kernel objects. Fixed releases skip missed periods rather than replaying them.

S3 owns GP20 calibration processing and buzzer feedback. After a complete valid calibration it ignores capture presses; the application owns GP20 start/reset and GP21 movement stop. Both readers use the existing button driver, with independent debounce state and no extra GPIO initialization.

## Local controls

Boot is CALIBRATING and S2's stop latch is set. Capture black then white using the [calibration workflow](LINE_TRACKING.md#calibration-workflow). Completion enters READY and discards prior GP20 debounce history. The calibration-completing press cannot start motion. Reboot is required to replace a completed calibration.

In READY, a fresh 30 ms release/press of GP20 on a fresh visible line starts FOLLOW_LINE. The boot latch must have a successful brake acknowledgement before reset; reset discards old requests. Starting on LOST, BROAD_BLACK or unusable guidance is rejected.

GP21 is delivered to `s2_stop()` before any snapshot/health read, dominates simultaneous GP20 and cancels calibration through S3. STOPPED retains valid calibration. Release GP21 and clear the cause; one fresh GP20 press resets to READY after acknowledged brake service. A separate release/press starts again. If stopped before calibration completes, reset returns to CALIBRATING. Held/bouncing or cancel-suppressed presses cannot restart.

Console transitions use `[LINE] state=CALIBRATING|READY|FOLLOW_LINE|RECOVER_LINE|STOPPED`; TEST transitions include `MOTOR_INHIBITED`. State transitions and software brake acknowledgement do not establish physical wheel motion/stationarity.

## Tracking and recovery

Public S3 steering is normalized −1…+1. Convert it to duty correction `0.10 * steering`, preserving the existing P calculation. FOLLOW_LINE requests left `clamp(0.10 + correction, 0, 0.35)` and right `clamp(0.10 - correction, 0, 0.35)`. Negative steering turns left; positive turns right. Equal readings request equal duties but do not prove physical centering. BROAD_BLACK during following requests 0.05 on both wheels; it does not identify a junction or finish.

Remember the last nonzero steering direction during the current run. On LOST, enter RECOVER_LINE with one wheel at 0.10 and the other zero in that direction; without a known direction, stop. The user-selected bench deadline is **500 ms from entry**, checked before reacquisition and never extended by new samples. Fresh valid sensing remains required throughout recovery.

Two consecutive, distinct visible acquisitions before the deadline resume following. Duplicate sequence/time cannot count twice. LOST or BROAD_BLACK resets the visible streak; broad black neither confirms reacquisition nor extends recovery. At elapsed time ≥500 ms, latch stop. This local reacquisition does not prove the original line or support obstacle/course context.

## Current interface contract

| Interface | Implemented behavior |
| --- | --- |
| `s3_ir_copy()` | Nonblocking coherent snapshot. Failure latches controller stop; cached data never gains a new acquisition time. |
| `MotionRequest` | Finite forward duty fractions 0…0.35, original `sampled_ms`, and `enabled`. Disabled requests brake without clearing a stop latch. |
| `s2_publish()` | Checked bounded latest-value copy; failure calls the independent stop path. Success is not measured movement. |
| Sensing freshness | Age ≤60 ms is usable; 61 ms is stale. Invalid acquisition/calibration, capture-active sensing or invalid visible guidance while moving stops. |
| `s2_stop()` | Persistent stop and independent Motion wake-up. Runtime callers share Motion's core 1. |
| `s2_reset_stop()` | Requires latest-stop brake acknowledgement; clears cached requests and advances the control epoch. Failure keeps STOPPED. Reset alone never commands movement. |
| `s2_health()` | Require successful read and meaningful service age ≤60 ms. Initial CALIBRATING permits up to 60 ms for the first braked service; this never permits a start without brake acknowledgement. Failed/expired health latches stop. |

Snapshot/health failures inhibit even when idle. Movement requests are emitted only in FOLLOW_LINE/RECOVER_LINE, with original paired acquisition time. Time/read/scheduling failures request stop before the controller task exits. S2 independently rejects expired/invalid requests and handles driver errors; it remains the exclusive PWM writer. Same-core stop serialization and acknowledged reset are implemented, but electrical brake timing and physical stopping are unmeasured.

## Build and acceptance

Use the four sequential [build profiles](../GETTING_STARTED.md#build-profiles). Integration fixtures INT-11…INT-14 compile only with TEST=1 and exercise private policy data; they never send live movement requests. Firmware check execution, raised-wheel motor polarity/braking and straight/curve/loss trials remain separate physical gates in the [integration log](../../testing/integration/TEST_LOG.md) and [testing procedure](../../testing/README.md).

Historical Phase 1–4 handoffs and fake-kernel results describe their recorded source, not the current runtime. Old `task_ir`, `robot_control`, `drv_ir` and replacement-calibration instructions are superseded by this guide. Saved logs remain unchanged. No motor failure cause, calibrated speed, measured stopping limit, or successful tracking run is inferred from compilation.

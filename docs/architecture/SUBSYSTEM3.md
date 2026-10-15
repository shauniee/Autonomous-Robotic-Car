# Subsystem 3: Line, Barcode, Mission and Safety

Owner: Nicholas. S3 senses line/barcode and coordinates Mission/Safety; S2 owns every motor output. [ARCHITECTURE](../ARCHITECTURE.md) owns system coordination and shared contracts. [TEAM](../../TEAM.md) owns wiring/resources.

## Line acquisition and calibration

The task reads paired ADC0/ADC1 at nominal 20 ms with one original acquisition time/sequence. Black/white capture uses at least 500 ms and 25 samples; each channel needs at least 300 ADC-count contrast. Missing/invalid acquisitions and gaps above 60 ms invalidate guidance. Completed references survive ordinary stop; reboot clears them.

The first accepted black press also posts a task-owned S4 calibration request. Keep braked/still while black and 100 fresh IMU samples complete. Black completion alone does not beep or permit white capture. Extra presses are ignored. One beep confirms both; three indicates cancellation/failure/deadline. A separate white press produces two beeps on accepted contrast. [GUIDE](../../GUIDE.md#every-power-on-calibrate-map-start) owns operator steps.

Normalization maps white=-1 and black=+1 using independently captured references. Line classification uses white enter/exit hysteresis and broad-black thresholds. Calibrated proportional guidance uses IR_KP=0.05, negative steering for left and positive for right; a guidance value is not movement permission. UNKNOWN/LOST/BROAD do not become guessed visible guidance. Snapshot copies preserve acquisition age and invalidate cached readings without refreshing their sequence/time.

## Barcode acquisition and decoding

GP16 DOUT edges use the shared hardware TIMER and real encoder capture. The ISR only appends bounded records; the task services at most 16 edges per release from a 64-edge ring. Widths use actual wheel count conversion, epoch, direction and wheel agreement. Capture/fifo overflow latches faults. The eight-command FIFO has one Mission consumer.

The provisional decoder supports Code 39 `*X*` for every A-Z letter in both orientations. Validate quiet zones, narrow/wide classes, gaps, framing, travel consistency and deadlines. An incomplete/bad/unknown frame cannot produce a navigation command. Valid unassigned symbols are reportable encounters only. Rearm requires configured quiet travel; bounds/polarity need measurements and issued-format confirmation.

Exactly four letters are assigned through MQTT, one per action. Defaults are all UNMAPPED. `s3_barcode_mapping_replace` atomically validates the full table; Mission separately enforces four-assignment semantics. Busy capture keeps its existing revision. Accepted encounter history survives reset/rejection independently of transient capture result and navigation FIFO; `s3_barcode_begin_run` clears reporting history for a fresh run.

| Interface | Contract |
| --- | --- |
| `s3_ir_copy` | Freshness-qualified coherent observation; no actuation |
| `s3_barcode_configure/reset` | Core-1 capture enable/reset, separate from movement |
| `s3_barcode_mapping_replace/copy/pop` | Bounded atomic replacement/snapshot/single-consumer FIFO |
| `s3_barcode_faults` | Independent fault read, no status lock needed |
| `mission_copy/mapping_submit` | Coherent Mission state and bounded stopped configuration handoff |

## Mission behavior

A valid barcode instruction is retained from its last-bar exit until the associated travel-qualified junction. Two commands, mismatched identities or expired travel/time context stop as ambiguous. Broad candidates advance only with consecutive distinct fresh IR sequences and acquisition timestamps; cached samples cannot accumulate dwell. Finish additionally requires the confirmed marker convention, action count and minimum travel.

Short loss recovery uses current-run steering history and a 500 ms absolute bound. It resets/rearms capture while retaining an already-qualified instruction; an additional unresolved queued command stops. Two fresh visible observations plus expected pose/corridor/heading permit resumption. New run/reset clears search direction. Obstacle preemption cancels pending navigation; unknown or nested route situations stop.

Safety priority 12 checks raw GP21, meaningful local progress, required data and faults independently of reporting. Mission priority 14 produces leased S2 actions. Fresh local GP20 start is separate from reset; configuration never starts movement. Watchdog and freshness policy are owned by ARCHITECTURE.

Modules: IR tracker/snapshot/calibration, `joint_calibration`, barcode decoder/service/runtime/adapter, `mission_guards`, Mission and Safety. [S3 cases](../../testing/subsystem3/TEST_LOG.md) and [integration](../../testing/integration/TEST_LOG.md) own evidence; source support is not physical tracking or alphabet acceptance.

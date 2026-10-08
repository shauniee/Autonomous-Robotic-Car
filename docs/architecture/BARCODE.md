> Runtime update, 8 October 2026: micro T-Kernel template imported. This subsystem remains a proposed full-mission feature and has no active driver/task in the IR bench. See [TEAM.md](../../TEAM.md) and [getting started](../GETTINGS_STARTED.md). Earlier scaffold file names below are historical and recoverable from the pre-migration snapshot.

# Barcode architecture: deferred S3 feature

Owner: Nicholas. Parent: [system architecture](../ARCHITECTURE.md). Wiring: [hardware](../HARDWARE.md). Review scope: FR2/FR3/FR9, Sections 3–6; trials T04/T05/T11/T14/T15.

## 1. Capture, decode and navigation contract

GP16 DOUT edge ISR captures bounded ordered transitions, timestamps and coherent encoder context independently of 20 ms line sampling. Decode **both black bars and white spaces**, using encoder distance or validated speed compensation. Calibrate comparator polarity/threshold, printed scale, traversal orientation and mounting. Identify complete frames and reject partial, unknown, malformed or overflowed captures rather than guessing a command.

Queue each mapped frame once in `Q_nav` with ID, symbol, action and acquisition time. Mission consumes only in valid mission context; reads during bypass/recovery may be logged but cannot silently redirect movement. Queue/capture overflow faults explicitly; purge obsolete events on reset/action completion.

Mission requests left/right/straight/U-turn as one ID-bound Motion action, with encoder completion and absolute timeout. Repeated lease renewal retains the action ID/deadline. Completion persists until acknowledged, then Mission reacquires the original line. Obstacle handling preempts navigation and cancels its prior action. Unknown course mapping/finish rules remain unresolved.

## 2. Current implementation and future states

The active two-sensor proportional IR bench has no GP16 barcode driver, capture ISR, decoder, navigation FIFO or encoder turns. The retired src/subsystems/line_barcode.c scaffold is historical, not a current file. U-turn and the other three encoder actions remain proposed Mission/S2 work. The later S3 refactor changes line steering only. Known A/Z captures, final course mapping, four-action accuracy, speed variation, duplicates and reverse/partial reads need tests before mission acceptance.

Future decoder states are disabled/unconfigured, wait quiet zone, capture frame, validate frame, queue event, wait for rearm, and fault. Partial/malformed/unknown/time-out frames are rejected outcomes; capture/queue overflow faults explicitly. DOUT HIGH/LOW becomes black/white only after measured polarity. No command mapping or finish rule is guessed. [IR_FUTURE_PLANS.md](IR_FUTURE_PLANS.md) defines staged work and planned checks. Source-grounded geometry and A/Z transcription below are retained.

## 3. Track geometry and sample technical reference



[Barcode & Line Specification.pdf p. 1](<../../../document/Barcode & Line Specification.pdf>) shows an **18 mm black line**, a **20 mm gap between the line edge and adjacent barcode**, and an **approximately 141 mm barcode length** along the illustrated direction. The measurements are on a photo; verify whether “gap” is from the line edge and whether 141 mm applies to all printed symbols on the physical track.

[Barcode Sample.pdf pp. 1–4](<../../../document/Barcode Sample.pdf>) visually contains examples labelled **A** and **Z** (two renderings each). It does not give a symbology name, start/stop rules, width tolerances, scale, printed decode table, or samples for B/C/D. Therefore do not hard-code an assumed Code 39/other standard or equate A/Z examples with the assignment's A/B/C/D command example without a verified specification.

**Visual sample transcription, so the PDF need not be reopened for the bar structure:** raster inspection at 72 dpi found **15 black bars** on each page, arranged as three five-bar groups separated by narrow spaces. Reading across the bars, `n` means a narrow bar/space and `w` a wide bar/space. Each nine-element sequence below alternates **black bar, white space, black bar, ... , black bar**. The first and last groups are the same in A and Z, consistent with `*` start/stop framing in Code 39; that standard identification is an **inference from the shapes**, not a label printed in the source. The PDF's A/Z text and the run widths are directly observed. [Barcode Sample pp. 1–4]

| Sample pages | Relative printed scale | First group | Labelled middle group | Last group | Inter-character gap |
| --- | --- | --- | --- | --- | --- |
| 1 (A), 3 (Z) | Larger, about 2× the narrow width of pp. 2/4 | `nwnnwnwnn` | A: `wnnnnwnnw`; Z: `nwwnwnnnn` | `nwnnwnwnn` | Narrow |
| 2 (A), 4 (Z) | Smaller | `nwnnwnwnn` | A: `wnnnnwnnw`; Z: `nwwnwnnnn` | `nwnnwnwnn` | Narrow |

At the smaller scale, measured raster widths are approximately **10 px narrow** and **30 px wide** at 72 dpi; the larger pages show approximately 20 px and 60 px. These are PDF drawing/raster dimensions, **not guaranteed millimetres on the course**. The sample pages have quiet white margins, but no quiet-zone minimum or barcode tolerance is specified. The sample provides **A and Z only**; no printed B, C or D examples or final command mapping are supplied. Nicholas confirmed the examples are relevant to the actual track, while the chosen command symbols remain uncertain.

**Geometry sketch in words:** looking down at the track image, the continuous **18 mm-wide** black line runs parallel to the barcode lane; a **20 mm white gap** separates the bottom of the barcode bars from the nearest edge of the line; the photographed barcode occupies about **141 mm along the travel direction**. The image does not specify the sensor mounting coordinate, barcode-to-junction distance, direction of travel, or whether every symbol has identical printed length. [Barcode & Line Specification p. 1]

**`DOUT` versus `AOUT` for barcode decoding:** a correctly adjusted comparator `DOUT` is sufficient to observe the alternating black/white sequence. `AOUT` gives reflectance strength and can help calibrate a difficult surface or lighting condition, but a valid decoder does not require an analogue channel. **Counting only the black bars is insufficient:** both supplied A and Z samples contain 15 bars. Their identity is in the *ordered narrow/wide widths of bars and intervening white spaces*. In the current plan, use barcode `DOUT` on Grove 4 GP16 and leave barcode `AOUT` disconnected. [Barcode Sample pp. 1–4; Robo Pico p. 4]

For a moving single-sensor barcode read, each `DOUT` transition marks a black/white edge; the time or **wheel-encoder distance between edges** gives each bar/space width. Time alone changes with speed, so encoder distance or speed compensation is needed to distinguish narrow from wide. Record raw transitions over known A and Z samples, calibrate `DOUT` polarity and threshold on the actual print, confirm orientation/height and printed dimensions, define frame start/end and invalid-read rejection, then emit **one** navigation event per valid symbol. Test at both slow and normal speeds and reject broken or incomplete scans. Mount the line sensors over the line and the barcode sensor over the adjacent barcode lane; exact lateral offsets require a measured car drawing. Three sensors total may constrain simultaneous line-position estimation and independent barcode reading.

The current plan uses analogue reflectance from the two line sensors on GP26/ADC0 and GP27/ADC1; the barcode sensor is digital on GP16. Pico W exposes only GP26–GP28 as external ADC channels, and GP28 is the left encoder A digital input here. There is therefore no third free onboard ADC channel for barcode AOUT without changing the pinout or adding hardware. Calibrate both analogue line channels on the actual surface, lighting, and mounting height; calibrate the barcode comparator separately. [Pico W p. 14; Robo Pico p. 5]

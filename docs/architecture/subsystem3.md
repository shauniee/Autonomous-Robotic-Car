# Subsystem 3: Line tracking and barcode

## Contents

- [1. Functionality](#1-functionality)
- [2. Line tracking](#2-line-tracking)
  - [2.1 Digital acquisition](#21-digital-acquisition)
  - [2.2 Local controls](#22-local-controls)
  - [2.3 Digital classification and guidance](#23-digital-classification-and-guidance)
- [3. Barcode](#3-barcode)
  - [3.1 Scope](#31-scope)
  - [3.2 Capture and encounter lifecycle](#32-capture-and-encounter-lifecycle)
  - [3.3 Widths and measured configuration](#33-widths-and-measured-configuration)
  - [3.4 Encoder and interrupt adapter](#34-encoder-and-interrupt-adapter)
- [4. Interfaces and contract](#4-interfaces-and-contract)
  - [4.1 Source modules](#41-source-modules)
  - [4.2 Line snapshot API](#42-line-snapshot-api)
  - [4.3 Barcode API](#43-barcode-api)
  - [4.4 Interface to S2 (Motion)](#44-interface-to-s2-motion)
- [5. Data and fault bits](#5-data-and-fault-bits)
- [6. Mission and Safety (summary; owned by architecture)](#6-mission-and-safety-summary-owned-by-architecture)
- [7. Status](#7-status)
  - [7.1 Implemented software](#71-implemented-software)
  - [7.2 Proposed or not established](#72-proposed-or-not-established)
- [8. Open items](#8-open-items)

- [Digital tracking revision (2026-10-31)](#digital-tracking-revision-2026-10-31)

## 1. Functionality

Owner Nicholas. Target S3 retains line acquisition, digital guidance and
barcode capture/decoding; Mission coordination moves to `app_program/mission/`.
No separate S3 or Safety task in target. [Plan](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117) owns work order,
[architecture](../architecture.md#mission-loop) owns Mission/controls/recovery,
[specs](../project-specs.md) owns course facts/data widths and
[team](../team.md#2-pins) owns wiring.

Two digital tracking sensors GP26 and GP27 follow the 18 mm line;
GP16 digital IR reads the left barcode lane. A single black barcode reading
cannot identify a character or junction. Read a complete frame using encoder
travel; continue line acquisition while capture is active. Keep decoding bounded
per Mission release and never wait for an entire print or manoeuvre.

## 2. Line tracking

### 2.1 Digital acquisition

DOUT GPIO sampling acquires both inputs from one register snapshot every
`IR_SAMPLE_MS`. Preserve acquisition time and reject invalid/stale readings.
No ADC acquisition, black/white references, capture accumulators or software
IR calibration are used. IMU boot calibration remains independent.

### 2.2 Local controls

GP20 starts when Mission prerequisites are met; GP21 stops/resets.
There is no IR calibration button sequence or calibration buzzer feedback.
Check each module's comparator threshold and `IR_BLACK_LEVEL` against actual
black/white surfaces. Wiring is owned by [team](../team.md#2-pins).

### 2.3 Digital classification and guidance

Left-only black steers right, right-only black steers left, both white drives
forward and both black immediately starts a queued direction; unqueued crossings retain qualification. Fixed
steering replaces line PID/filter/ramp; Motion retains wheel-speed PID.
UNKNOWN means invalid/stale input. White/white cannot distinguish centred
travel from a lost line. Existing junction action and stop policies remain.

## 3. Barcode

### 3.1 Scope

Decode all A-Z in either print traversal direction as the same character. Use
Code 39 as selected design, with actual framing/print tolerances verified under
D01. Four distinct action letters are received over MQTT; no compiled mission-day
mapping. Decoder patterns may be compiled. Missing mapping blocks start.
Incomplete/noisy captures must not guess characters; no complex damaged-print
recovery is required for the expected intact course prints.

### 3.2 Capture and encounter lifecycle

Reuse the bounded quiet-zone/capture/rearm decoder and static edge/event storage.
Each encounter yields at most one action; direction reversal and overflow reject
untrustworthy partial context. No two-second global scanner blind delay. Revisit
of a print after a completed encounter can produce a new command; exact rearm
distance remains D01. Event orientation is evidence, not a reversed action mapping.
Enabling capture queues the current sensor level and coherent wheel counts so
the first print can use the white approach as its quiet-zone reference. The
quiet-zone distance must still be travelled after enabling; starting on a print
does not authorize decoding a partial frame. Either print orientation is accepted
while driving forward; backward wheel travel continues to reject the frame.

### 3.3 Widths and measured configuration

Bar widths come from calibrated encoder displacement, not elapsed time, PWM or
bar count. Measure polarity, narrow/wide bounds, quiet zone, print scale, sensor
position and barcode/junction spacing. Existing default constants are provisional.
Validate both directions at operating speeds and distinguish capture failure
from an unsupported inferred letter. Pending D01/D03/D04.

### 3.4 Encoder and interrupt adapter

Reuse `drv_barcode` behind the shared core-1 GPIO dispatcher, never replace encoder
or Echo handlers. ISR captures level and coherent wheel counts then returns;
decoding occurs in bounded Mission work. Retain count wrapping/epoch assumptions
from current accessors and reject lost capture order. Verify hardware initialization
and enable path rather than claiming a host decode proves physical capture.

## 4. Interfaces and contract

### 4.1 Source modules

`subsystem3.c/.h` owns bounded `s3_step` and a line snapshot;
`drv_ir_tracking` and `ir_tracking` acquire/classify; `drv_barcode`, `barcode_decode`
and `barcode` capture/decode/map events. `app_program/mission/` now owns the
coordinator and bounded safety helpers; no input/Safety task is created.

### 4.2 Line snapshot API

`s3_ir_copy` currently copies under a private mutex; preserve validity and original
sample time. A successful copy is not fresh/usable sensing. Mission-local copies
may be simplified after consolidation, while core-0 Reporting still needs coherent
bounded access; do not remove synchronization merely because one task was merged.

### 4.3 Barcode API

Reuse `barcode_mapped`, mapping validation, start/stop and `barcode_pop` with one
consumer. Current capacities: 64 captured edges, eight decoded events; overflow
is explicit, not permission to replay an older action. Target Mission owns mapping
installation and navigation; core-0 I/O submits mapping requests asynchronously.
ACK follows installation. Stop drops pending actions, retains mapping/display.

### 4.4 Interface to S2 (Motion)

Mission is the only command producer. S3 never writes motors. See the central
[mode-dependent contract](../architecture.md#motion-command-contract) rather than
maintaining a second list of stop/freshness rules.

## 5. Data and fault bits

Keep packed fields/accessors and historical [ADR 0001](../adr/0001-base-2-scaled-integers.md).
Target seven Mission states plus invalid fit three logical bits while retaining
the existing four-bit wire slot; review numeric state/decoder compatibility.
Line/Obstacle/Mission fault bytes become Mission-written; preserve wire positions.
Policy: [fault table](../architecture.md#fault-bitmask).

## 6. Mission and Safety (summary; owned by architecture)

[Architecture](../architecture.md#mission-loop) is authoritative. Barcode is held
to the next junction, consumed once and followed onto the selected outgoing route.
U-turn pauses about 200 ms while stopped then turns once; no turn is undone.
Safety becomes small Mission/Motion functions; Motion independently enforces stop.
Fresh digital guidance/mapping are READY prerequisites; no CALIBRATE/COMPLETE target states.

## 7. Status

### 7.1 Implemented software

At remote `5e6a45b`, input/calibration, P guidance, barcode decoder/service,
Mission/Safety and their separate task startup paths exist. Local `4b00e20`
adds line-loop/crossing/timing-related changes; those are not remote-main behavior.
Preserve [S3 results](../../testing/subsystem3/TEST_LOG.md) exactly; neither inventory
nor saved host results claims a final-image physical run.

### 7.2 Proposed or not established

The working three-task integration and GP21-release controls are implemented and
software-tested. Requested sweep geometry, mounted junction confirmation and
end-to-end bidirectional barcode navigation still need decisions/calibration.
White/white is no longer assumed to prove centering. Standalone recovery stops
and pivots through a wider bounded sweep; see the central recovery contract. Outgoing turn confirmation
counts distinct acquisitions; U-turn pause is measured from encoder stationarity
in software, with its physical reference still D02/D08. Do not describe all Motion/Mission code as absent
or reuse historical bench statements as present status.

## 8. Open items

D01/D02/D03/D04/D07/D13 own missing values. Validate MR-12/13/26/27/28/32, T03/T04/
T05/T11/T14/T15; test command-once, loop junction re-entry, partial capture rejection,
stale producers and stop during every phase. Allocate new case IDs before use;
never rewrite existing S3-BF or other evidence for this target.

Continuation correction: junction qualification counts distinct usable black
acquisitions and travel before the first black exit. Travel
after black ends cannot turn a thin patch into a confirmed
junction or consume its held barcode. Qualified straight/unmapped crossings
use the straight-segment revision above before ordinary tracking. All A-Z now
have both-orientation/two-scale host fixtures; physical print/mounting tests
remain pending.

## Digital tracking revision (2026-10-31)

Line tracking now samples DOUT on GP26 (left) and GP27 (right) as GPIO.
Disconnect AOUT before connecting DOUT. `IR_BLACK_LEVEL` is 1 (HIGH black, LOW white), changed after both-white
was reported as broad black with the previous LOW-black setting;
verify polarity with each module over black and white and adjust its comparator
potentiometer. This wiring/polarity is a required change, not an observed pass.
No software black/white capture is needed; GP20 is available for local start.

Left-only black steers right, right-only black steers left, both white drives
forward and both black immediately starts a queued direction; unqueued crossings retain qualification. White/white cannot distinguish centred travel from losing the line with this
sensor arrangement. Stale/invalid acquisition still stops through existing safety.

Line PID/filter/ramp history is removed; wheel-speed PID remains in Motion.
Normal tracking is 70 mm/s (double the prior 35). A single black sensor
requests full differential correction: inside wheel 0, outside wheel 140 mm/s.
This is provisional physical tuning, not a measured curve-tracking pass.
With no queued instruction, broad-black entry holds half tracking speed (35)
until qualified black exit. Queued directions enter navigation on the first
fresh usable both-black sample, without waiting for travel or black exit.
Straight navigation ramps from 35 to 70 over its 25 mm exit distance.
Left/right/U-turn navigation ramps from 35 to 70 over the combined pivot wheel
arc and 60 mm clear distance; it does not restart at half speed for the clear
phase. The U-turn's stationary pause remains. Ramp progress comes from signed,
wrap-safe encoders and is clamped to 0..1; elapsed time without motion cannot
increase speed. Confirmation uses full tracking speed, then tracking resumes.
Motion retains its approach braking at action endpoints, so the ramp is a
requested speed ceiling, not a promise of constant physical acceleration or
full measured wheel speed at the pivot endpoint. Hump reductions remain.
No-command junction qualification, action limits, stop and obstacle policies remain.
IR calibration/capture state and APIs are removed entirely. Digital endpoint
fields retain their Q7 scale for steering consumers, with no stored references.
These paragraphs supersede analogue tracking descriptions earlier in this guide.

Mounted steering correction (2026-10-31): with pins confirmed correct, the user
observed left-black turning left and right-black turning right. Mission now
maps left-black error -127 to steering -127 and right-black +127 to +127,
reversing the previous correction. The shared mapping also feeds bypass line
following. GPIO assignments, normalization and queued-junction handling remain.
Physical confirmation of this change is pending.

# Subsystem 3: Line tracking and barcode

## Contents

- [1. Functionality](#1-functionality)
- [2. Line tracking](#2-line-tracking)
  - [2.1 Acquisition and calibration bounds (implemented in software)](#21-acquisition-and-calibration-bounds-implemented-in-software)
  - [2.2 Calibration workflow](#22-calibration-workflow)
  - [2.3 Normalization, classification and guidance](#23-normalization-classification-and-guidance)
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

## 1. Functionality

Owner Nicholas. Target S3 retains line acquisition, calibration, guidance and
barcode capture/decoding; Mission coordination moves to `app_program/mission/`.
No separate S3 or Safety task in target. [Plan](../../plan.md) owns work order,
[architecture](../architecture.md#mission-loop) owns Mission/controls/recovery,
[specs](../project-specs.md) owns course facts/data widths and
[team](../team.md#2-pins) owns wiring.

Two analogue tracking sensors GP26/ADC0 and GP27/ADC1 follow the 18 mm line;
GP16 digital IR reads the left barcode lane. A single black barcode reading
cannot identify a character or junction. Read a complete frame using encoder
travel; continue line acquisition while capture is active. Keep decoding bounded
per Mission release and never wait for an entire print or manoeuvre.

## 2. Line tracking

### 2.1 Acquisition and calibration bounds (implemented in software)

Reuse paired ADC acquisition and bounded conversion timeout/polling. Do not publish
a half-updated pair. Preserve original sample time; failed acquisition and failed
calibration produce explicit invalidity, not a plausible zero reading. Tunables
live in `robot_config.h`; current periods/thresholds are not calibrated target limits.
Raw range and Q7 fractions/error are defined in the [data table](../project-specs.md#5-data-representation-table-week-13).

### 2.2 Calibration workflow

Target: stationary boot IMU calibration attempt completes/fails visibly before
IR calibration is enabled. IR black capture then white capture uses GP20 while
stopped, one chirp then two for an accepted pair (rejection three chirps).
Calibrate each mounted sensor independently under course lighting. Keep motion
inhibited throughout. GP21 cancels/stops. Local stop keeps valid references;
reboot clears RAM-only references. Target GP20 starts only after prerequisites;
the current software workflow is in README.

### 2.3 Normalization, classification and guidance

Preserve normalized black fractions, signed two-sensor error and explicit UNKNOWN,
VISIBLE, LOST and BROAD_BLACK classification. Thresholds remain provisional D04/D07.
UNKNOWN from failed/stale acquisition differs from fresh white/white loss.
Line guidance and S2 wheel PID are different loops; preserve existing guidance
until tuning is verified, and never call P-only code physically validated PID.
Remote main guidance is P-based; the local checkpoint adds Mission line-loop tuning.

At a confirmed no-command junction request at least 25 mm straight crossing travel,
then track/acquire outgoing line. BROAD_BLACK is evidence, not automatic finish
or sufficient proof of junction centre. Qualification uses mounting and encoder
travel, pending D02/D04/D07. Target line loss enters bounded moving sweep recovery;
it does not universally latch on the first white sample.

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
Calibration/mapping are READY prerequisites; no CALIBRATE/COMPLETE target states.

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
White/white is no longer assumed to prove centering; a known-side bounded arc is
retained provisionally, and unknown-side loss stops. Outgoing turn confirmation
counts distinct acquisitions; U-turn pause is measured from encoder stationarity
in software, with its physical reference still D02/D08. Do not describe all Motion/Mission code as absent
or reuse historical bench statements as present status.

## 8. Open items

D01/D02/D03/D04/D07/D13 own missing values. Validate MR-12/13/26/27/28/32, T03/T04/
T05/T11/T14/T15; test command-once, loop junction re-entry, partial capture rejection,
stale producers and stop during every phase. Allocate new case IDs before use;
never rewrite existing S3-BF or other evidence for this target.

Continuation correction: junction qualification counts distinct usable black
acquisitions and travel before the first black exit. Completing the forced
minimum crossing after black ends cannot turn a thin patch into a confirmed
junction or consume its held barcode. Qualified straight/unmapped crossings
also enter outgoing-line confirmation before ordinary tracking. All A-Z now
have both-orientation/two-scale host fixtures; physical print/mounting tests
remain pending.

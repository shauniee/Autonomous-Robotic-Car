# System architecture

This is the target design for `main`. It implements [PROJECT_SPECS](PROJECT_SPECS.md) and [CHANGE-REQUIREMENT](CHANGE-REQUIREMENT.md) with the fewest mechanisms that meet them. [TRIBUNAL1](TRIBUNAL1.md) records the Week 8 plan: task ownership, priorities, RTOS interfaces and authority boundaries are unchanged from it. The [post-Tribunal decisions](#post-tribunal-decisions) below simplify four of its mechanisms. Pre-realignment code remains on branch `legacy-main`; the fuller but overengineered integration remains on branch `uat` as a port source.

## Tasks

All application tasks run on core 1. The kernel's WiFi/lwIP/MQTT task (`cyw43_utk_task`, priority 4) runs on core 0. Smaller numbers mean higher priority.

| Task | Owner | Priority | Release | Role |
| --- | --- | --- | --- | --- |
| Safety | Nicholas | 12 | 10 ms | GP21 stop latch, fault aggregation, freshness checks, stop authority |
| Motion | Mahesha | 13 | 20 ms, or woken by stop | Sole motor writer; encoders, wheel PID, distance/turn actions |
| Mission | Nicholas | 14 | 20 ms | Navigation decisions, state machine, mapping acceptance |
| Line/Barcode | Nicholas | 15 | ADC 20 ms; GP16 edge IRQ | IR line snapshot, barcode capture and decode |
| Terrain | Darrius | 16 | 20 ms | IMU calibration, pitch, humps, impact |
| Obstacle | Jia Jing | 17 | Echo IRQ; trigger every 60 ms | Forward range, servo scans, profiles, side-watch |
| Reporting | Shaun | 20 | Collect 20 ms; publish 500 ms | Binary telemetry, event record, snapshot replies, mapping hand-off |

## Authority

- Motion alone writes motor PWM.
- Mission owns navigation decisions and accepts mapping changes, only while stopped.
- Safety holds stop authority independent of Mission and Reporting.
- Line/Barcode, Terrain and Obstacle provide sensor evidence and never write motors.
- Networking has no movement authority; network loss never stops otherwise healthy local autonomy.

```mermaid
flowchart TD
    LB[Line/Barcode] -->|IR word, barcode events| Mission
    Terrain -->|terrain word| Mission
    Obstacle -->|range word, profile| Mission
    Mission -->|scan / side-watch request| Obstacle
    Mission -->|motion command| Motion
    Motion -->|feedback word, encoder counts| Mission & LB & Terrain & Obstacle
    Safety -->|stop event flag| Motion
    LB & Terrain & Obstacle & Motion -->|own fault byte| Safety
    Mission & Motion & LB & Terrain & Obstacle -->|snapshots| Reporting
    Reporting <-->|binary frames| Kernel[Kernel MQTT adapter, core 0]
    Kernel -->|mapping command| Reporting -->|mapping| Mission
```

## Shared-data rules

Exactly three sharing patterns are used. Each storage unit has one writer; only that writer performs read-modify-write on it.

1. **Word snapshot.** A producer packs a snapshot of at most 32 bits into one aligned `uint32_t`, composing it in a register and publishing it with a single store. Readers take a single load and decode with accessor functions. An aligned 32-bit load/store is single-copy atomic on RP2040 across both cores, so no lock is needed and no reader sees a torn snapshot.
2. **Mutex copy.** Snapshots larger than 32 bits (obstacle profile, event record, telemetry aggregate) are copied under one priority-ceiling mutex. The lock covers only the bounded copy.
3. **ISR-to-task ring.** Interrupts push timestamps into a single-producer/single-consumer ring (barcode edges, Echo edges). Scalars shared with an ISR on core 1 use a short `DI`/`EI` section.

No other mechanism is used: no custom PRIMASK code, no `volatile` as synchronisation, no lock-free retry loops. Fields are never read in place from another subsystem's storage; C cannot address a bit-field, and in-place reads of multi-field data are not coherent.

### Fault bitmask

The shared fault bitmask is stored as one byte per owner. Each owner sets and clears only the bits in its own byte, so no two writers share a storage unit. Bits are defined in [DATA_REPRESENTATION](DATA_REPRESENTATION.md).

| Byte owner | Example bits |
| --- | --- |
| Motion (S2) | motor stall, encoder fault |
| Line/Barcode and Mission (S3) | IR invalid, line lost, barcode capture overflow, barcode event overflow |
| Terrain (S4) | IMU fault, calibration failed, impact |
| Obstacle (S5) | no echo, servo fault |
| Network (kernel task, core 0) | Wi-Fi lost, broker lost (report-only; never stops motion) |

Safety reads every byte each release, latches stop for motion-relevant bits and publishes the combined mask for telemetry. Simultaneous faults stay independently visible.

## Motion command contract

- Mission writes one packed command each 20 ms: mode (stop, follow, distance, turn), speed and steering or target, a small action ID and the acquisition time of the sensor sample it was derived from.
- Motion brakes when that sample is older than the configured age limit (D07). Copying or rewriting a command never refreshes its sample time.
- Discrete actions report `IDLE`, `RUNNING`, `DONE` or `FAILED` with their action ID in Motion's feedback word. Mission starts the next action after seeing `DONE` for its ID; a new command replaces the old one. Only Motion writes the status.
- Stop: Safety or GP21 sets a persistent event flag and wakes Motion, which brakes in that release. Reset needs a fresh local GP20 press with the cause cleared, and never starts motion on its own. Zero commanded speed is not proof of stopping; stationarity comes from encoder counts.

## Encoder contract

- The encoder ISR keeps one fixed-width wrapping count per wheel. The width is set from measured counts per revolution and the longest segment any consumer measures (D03).
- Each consumer (Motion actions, barcode bar widths, hump travel, bypass legs) stores its own origin and computes travel by modular subtraction. Nobody resets the shared count, so travel length is unbounded and consumers stay independent.

## Subsystem behaviour

### Line and barcode (S3)

- Two analogue IR channels (GP26/GP27) are calibrated black/white and published as one word snapshot: normalised readings, line state and validity.
- The GP16 edge ISR pushes edge time and encoder position into a ring. The task measures bar widths from encoder travel, decodes Code 39 A–Z in either direction and queues decoded events (small fixed depth; overflow sets a fault bit).
- The runtime mapping holds four 5-bit letters (left, right, straight, U-turn). Boot state is unmapped.

### Mission (S3)

- One `switch` on the state with one handler function per state. Candidate states: Calibrating, Ready, FollowLine, Navigation, Scan, Bypass, Reacquire, Stopped, Fault, Complete. The final list and its bit width are set in the Mission issue.
- A decoded barcode command is held until the next cross junction, then executed as a Motion action.
- Line loss allows a bounded recovery before stopping.
- Arbitration: stop/fault, then required-data freshness, then obstacle, then navigation, then line following.

### Obstacle and bypass (S5 with Mission)

- The forward range is a word snapshot; no echo is unknown, never clear space.
- When an obstacle blocks the line, Motion stops and Obstacle performs a stationary coarse scan (five rays) and a focused fine scan, producing location, width, closest point and left/right clearance.
- Side-watch bypass: turn toward the side with more clearance, drive an offset leg, turn parallel, then drive the parallel leg with the servo held toward the obstacle. A range jump marks pass-by; encoder travel along the leg gives obstacle depth. Turn back, return the offset and reacquire the original line using expected heading and two fresh line observations. A second obstacle during bypass stops the robot.

### Terrain (S4)

- Calibrate accelerometer bias while stationary at start.
- Height accumulates sin(pitch) multiplied by encoder travel through each climb, crest and descent. Working values are 16-bit fixed point; the reported height is packed to 7 bits with a defined scale and invalid code.
- Only a valid completed encounter adds a hump event and may update the maximum. An impact sets an S4 fault bit.

### Communications (S1)

- The kernel adapter (`lib/liblwip/sysdepend/pico_rp2040/mqtt_utk.c`) only connects, reconnects, publishes byte frames and hands received frames to Reporting. It contains no application logic and includes no application headers beyond one S1 bridge header.
- Three topics: telemetry (binary, every 500 ms and on snapshot request), command (binary snapshot request and mapping), status (retained online/offline last will and the mapping acknowledgement).
- Frame layout (version/type header, little-endian, explicit masks and shifts, field order, scales, validity, length and version checks) is defined in `docs/MQTT_PROTOCOL.md`. Native structs are never sent.
- Mapping acknowledgement `{sequence, result}` means the complete mapping was validated and installed; receipt alone is not an acknowledgement.

## Time

- Kernel milliseconds drive freshness and control; the hardware TIMER microseconds time barcode and Echo edges. Each domain stays explicit.
- Timestamps may be stored narrow; all age calculations use unsigned wrap-safe subtraction, and each width is justified in [DATA_REPRESENTATION](DATA_REPRESENTATION.md).
- Storage, waits, retries and actions are bounded. ISRs capture, signal and return.

## Post-Tribunal decisions

Agreed on 2026-10-25 as change type "Simplified". Each keeps the Tribunal's intent while removing machinery that the requirements do not need.

| Tribunal 1 record | Adopted design | Reason |
| --- | --- | --- |
| Leased Motion commands renewed without moving the original sensor time; results retained until acknowledged | Timestamped command rewritten every 20 ms; Motion brakes on stale sample time; DONE/FAILED with action ID; no renewal, epochs, acknowledgement or two-phase reset | One producer and one consumer on one core. Staleness is the only hazard a lease guards against, and the sample time already covers it. A single status writer removes the acknowledgement race. |
| Full 26-entry mapping replacement with four acknowledgement slots | Four 5-bit letters (20 bits) and one `{sequence, result}` acknowledgement | The rule announces exactly four letters. 3 bytes instead of 26 entries, and one acknowledgement state instead of a queue. |
| Encoder distance reset at every barcode | Fixed-width wrapping counts with per-consumer origins | A missed barcode leaves the interval unbounded, and a shared reset breaks other consumers (Tribunal Version C gap 3). |
| Mutex-protected copies for every snapshot | One-word snapshots by single store/load; mutex copies only above 32 bits | Atomic on RP2040 across cores, with no lock time and no torn reads. Meets risks R1 and R4 with a single writer. |

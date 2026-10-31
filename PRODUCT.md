# Product

<!-- impeccable:product-schema 1 -->

## Contents

- [Platform](#platform)
- [Users](#users)
- [Product Purpose](#product-purpose)
- [Operating Context](#operating-context)
- [Capabilities and Constraints](#capabilities-and-constraints)
- [Evidence on Hand](#evidence-on-hand)
- [Product Principles](#product-principles)

This file covers the laptop telemetry dashboard (`scripts/dashboard.html`), the project's only UI surface. Robot requirements live in [project-specs](docs/project-specs.md); protocol and authority in [architecture](docs/architecture.md).

## Platform

web

## Users

The CSC01 team on the bench: members testing, calibrating and debugging the Pico WH robot from a laptop beside it. Assessors and video viewers are secondary; the dashboard is not required for marking ([project-specs](docs/project-specs.md): "a fancy dashboard is not required").

## Product Purpose

At a glance, show whether the robot is healthy and what each subsystem is doing:

- Mission state
- IR line-tracking status
- Obstacle detection status
- Terrain (hump) status
- Motor status
- Active faults and errors

Wheel diagnostics compare requested speeds, encoder-measured release speeds and
serviced duty. Motor gains remain compile-time constants; the procedure and
calibration responsibilities live in [S2](docs/architecture/subsystem2.md#wheel-speed-tuning).
The dashboard observes the robot and installs barcode mappings; it does not tune
gains or start motors remotely.

Readings are shown as the decoder normalises them from the binary frame into base-10 values a person can read (for example 0-100 scales), not as raw sensor samples.

## Operating Context

- Served locally by `python3 scripts/mqtt_client.py serve` at `http://127.0.0.1:8080`; decoded frames arrive by Server-Sent Events.
- Robot publishes compact binary MQTT telemetry (schema v2, 23-byte frame, 500 ms stream) plus events, acknowledgements and best-effort wheel traces to a laptop broker on a private 2.4 GHz lab LAN; on demonstration day it uses the instructor's broker.
- Physical controls are local: GP20 start, GP21 stop/reset. Mapping (four mission-day letters) is accepted while stopped; next-junction tests follow the existing Mission gate. Authority is owned by [architecture](docs/architecture.md#authority).
- Readings are taken mid-test, often standing, with attention split between the robot and the screen.

## Capabilities and Constraints

- **Single static file:** one `dashboard.html`, no build step, no external dependencies.
- **Offline:** no CDN fonts, scripts or images; the lab LAN may have no internet.
- **Desktop only:** used in a desktop browser on the team's laptops; phones and tablets are not supported targets.
- **Authority:** networking never starts, stops or drives the robot ([architecture](docs/architecture.md#authority)). Only Motion writes the motors.
- **Terminology:** use the firmware names — Mission states `READY`, `FOLLOW_LINE`, `NAVIGATE`, `SCAN`, `BYPASS`, `REACQUIRE`, `STOPPED`; Motion `IDLE`, `RUNNING`, `DONE`, `FAILED`; fault names as decoded by `mqtt_client.py`.

- **Tuning lives apart from the mission build:** the team's understanding is that the mission firmware must not accept laptop-to-robot tuning data over MQTT. Live tuning is therefore a concept for a separate build or script, kept out of the mission firmware and the normal dashboard flow.

Open decisions (not implemented; do not present as working):

- **Tuning build/script:** which constants are tunable, their safe ranges, when the robot accepts them (stopped only?), how values return to `robot_config.h` once settled, and the transport all need a firmware and [architecture](docs/architecture.md) decision first.

## Evidence on Hand

- Existing dashboard: `scripts/dashboard.html` (single-screen dark ledger, Mission state vocabulary, S1–S5 health, sensor readings, custom barcode mapping, wheel-speed chart and event log).
- Decoder and server: `scripts/mqtt_client.py` with golden vectors shared with `testing/pure/test_mqtt_frame.c`.
- Synthetic dashboard screenshot fixtures and host results live under `testing/integration/logs/`; they are not physical observations. Do not invent readings, calibrated limits or thresholds; unknown values belong in [pending decisions](docs/project-specs.md#pending-decisions).

## Product Principles

1. **Health first:** a stopped robot, active fault or stale stream must be obvious before any number is read.
2. **Show what the robot reported, not what it might mean:** no invented heights, speeds or states beyond the decoder's derivations.
3. **Observe freely, tune separately:** the mission dashboard observes; tuning belongs to a clearly separate developer build, and any value it sends is explicit, acknowledged and never commands motion.
4. **Bench-proof:** works offline from one file on any team laptop.

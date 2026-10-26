# CSC01 Autonomous Robotic Car

An autonomous Pico WH / Robo Pico robot for line tracking, barcode navigation, hump measurement, obstacle avoidance and WiFi telemetry. Firmware uses micro T-Kernel 3.0 on the RP2040. See [project specifications](docs/project-specs.md) for required outcomes and [team](docs/team.md#6-getting-started) for what runs today.

## Reading order

| Guide | Use it for |
| --- | --- |
| [Project specifications](docs/project-specs.md) | Requirements, change requirement (minimum-bit), data table, assessment, pending decisions |
| [Team](docs/team.md) | Owners, pins, peripherals, hardware reference, getting started, coding rules, workflow |
| [Architecture](docs/architecture.md) | Tasks, authority, shared-data rules, MQTT protocol, subsystem navigation |
| [Subsystem guides](docs/architecture/) | `subsystem1.md` comms, `subsystem2.md` motion, `subsystem3.md` line/barcode/mission, `subsystem4.md` terrain, `subsystem5.md` obstacle |
| [Tribunal 1 summary](docs/additional-docs/tribunal1.md) | Week 8 defence, decisions and gaps |

Each fact has one owning guide; other guides link to it. Assignment requirements, proposed design, implemented behavior and recorded results are distinct.

## Repository structure

| Path | Contents |
| --- | --- |
| `app_program/` | `app_main.c` startup, shared definitions and five subsystem folders; application drivers stay with their subsystem |
| `build_make/` | Make profiles and generated firmware |
| `config/` | Kernel/device configuration and ignored local credentials |
| `kernel/`, `lib/`, `device/`, `include/`, `tools/` | Imported runtime, services, drivers, headers and conversion tools |
| `docs/` | Three project guides, five subsystem guides and one additional document |
| [testing/](testing/README.md) | Stable test IDs, result tables and saved evidence |
| [videos/](videos/README.md) | Unlisted demonstration links |

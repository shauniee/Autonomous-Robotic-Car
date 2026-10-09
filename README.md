# CSC01 Autonomous Robotic Car

An autonomous Pico WH / Robo Pico robot for line tracking, barcode navigation, hump measurement, obstacle avoidance and WiFi telemetry. Firmware uses micro T-Kernel 3.0 on the RP2040. See [project specifications](docs/PROJECT_SPECS.md) for required outcomes and [getting started](docs/GETTING_STARTED.md#current-capabilities) for what runs today.

## Reading order

| Guide | Use it for |
| --- | --- |
| [Getting started](docs/GETTING_STARTED.md) | Install, build, flash, calibrate, operate and complete project tasks |
| [Team](docs/TEAM.md) | Owners, wiring, peripheral allocation, priorities and workflow |
| [Project specifications](docs/PROJECT_SPECS.md) | Requirements, deliverables, assessment, course details and pending decisions |
| [Architecture](docs/ARCHITECTURE.md) | System coordination, data contracts and subsystem navigation |
| [Technical information](docs/TECHNICAL_INFO.md) | Hardware limits, interfaces, registers and formulas |
| [Coding standard](docs/CODING_STANDARD.md) | Project C rules and review checklist |
 |

Each fact has one owning guide; other guides link to it. Assignment requirements, proposed design, implemented behavior and recorded results are distinct.

## Repository structure

| Path | Contents |
| --- | --- |
| `app_program/` | `app_main.c` startup, shared definitions and five subsystem folders; application drivers stay with their subsystem |
| `build_make/` | Make profiles and generated firmware |
| `config/` | Kernel/device configuration and ignored local credentials |
| `kernel/`, `lib/`, `device/`, `include/`, `tools/` | Imported runtime, services, drivers, headers and conversion tools |
| `docs/` | Six project guides, seven architecture guides and preserved drawings |
| [testing/](testing/README.md) | Stable test IDs, result tables and saved evidence |
| [videos/](videos/README.md) | Unlisted demonstration links |

Learn Nicholas’s complete implementation through [SUBSYSTEM3_GUIDE](docs/SUBSYSTEM3_GUIDE.md), including configuration, public APIs, barcode navigation and startup behavior.

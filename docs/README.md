# Documentation index

Primary integrated design: [CSC01 Design Review](<../../CSC01 Design Review.docx>), preserved unchanged. [Consistency audit](DESIGN_REVIEW_AUDIT.md) records source conflicts and corrections.

| Read | Purpose |
| --- | --- |
| [Project information](project-info.md) / [plan](project-plan.md) | Assignment scope, ownership and implementation sequence |
| [Architecture](ARCHITECTURE.md) | Review’s target tasks, safety and communication contracts |
| [Hardware](HARDWARE.md) / [technical reference](tech-info.md) / [machine summary](MACHINE.MD) | Wiring, electrical/data evidence and reported components |
| [Implementation](IMPLEMENTATION.md) / [right IR guide](RIGHT_IR_LINE_FOLLOW.md) | Current code flow, limitations, flash/calibration and tuning |
| [Getting started](GETTING_STARTED.md) | Reading and operation order |
| [Sensor bench](SENSOR_BENCH_MQTT.md) | Optional stationary acquisition target |
| [WiFi/MQTT guide](WIFI_TELEMETRY.md) | Current connection, subscription and reporting contract |

Project/technical Markdown lives here; repository README and root AGENTS remain short entrypoint/instruction files.

## Build from the repository root (`code/`)

```sh
cmake -S . -B build-right-ir-follow -DPICO_BOARD=pico_w -DPICO_SDK_PATH=/path/to/pico-sdk
cmake --build build-right-ir-follow --target right_ir_follow_mqtt -j 6
```

Flash `build-right-ir-follow/right_ir_follow_mqtt.uf2`. SDK/toolchain setup is required; BOOTSEL mass-storage copying is separate from USB serial monitoring. Use the [operation guide](RIGHT_IR_LINE_FOLLOW.md) for calibration and local controls.

Host checks: `sh scripts/check-sensor-bench.sh` on macOS/Linux, or `powershell -ExecutionPolicy Bypass -File scripts/check.ps1` with GCC on Windows. These do not prove physical tracking or mission completion.

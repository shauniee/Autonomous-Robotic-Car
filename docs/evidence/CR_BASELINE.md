# CR baseline (before): main and uat

Issue #22. Measured from pinned commits, not copied from earlier notes.

## Sources

| Build | Commit | Source |
| --- | --- | --- |
| `main` | `c6c4ef9` | pinned commit |
| `uat` | `d8220bc` | pinned commit |

Both trees use the same `build_make/makefile` and `build_make/pico_rp2040.mk` (sha256 `058ec5e1…a5010f` and `72771382…369a74`), taken from `main` at `ea9c164`. Only the source differs between the two builds.

## Toolchain and command

Arm GNU Toolchain 15.3.Rel1, `arm-none-eabi-gcc 15.3.1 20260627`. Host: macOS 25.5.0, GNU make 3.81. Pico SDK 2.3.1 (copied from the user's SDK tree).

Run in each tree's `build_make/`:

```sh
make clean
PICO_SDK_PATH=<sdk> make -k -j8 SMP=1 CONSOLE=usb_cdc WIFI=none NET=none MQTT=0 CRASH_REPORT=0 TEST=0
arm-none-eabi-size mtk3pico_smp1_usb_cdc.elf
```

## Results

| Build | Warnings | Undefined refs | text | data | bss |
| --- | ---: | ---: | ---: | ---: | ---: |
| `main` `c6c4ef9` | 11 | 0 | 60564 | 560 | 25208 |
| `uat` `d8220bc` | 11 | 0 | 86196 | 560 | 26540 |

The 11 warnings are in the pinned sources, not in the flags. They are part of the baseline.

## CR-scope struct sizes

Command: a probe file with `const unsigned long sizeof_X = sizeof(X);`, compiled with `arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -ffreestanding -std=gnu11 -O2 -DCNF_SMP=1 -DAPP_TEST=0 -DTM_CONSOLE_UART=0 -DBOARD_UART0_FREE=1 -D_PICO_RP2040_ -I include -I config -I kernel/knlinc -I . -S`, value read from `.word` in the `.s` output.

| Struct | `main` (bytes) | `uat` (bytes) |
| --- | ---: | ---: |
| `S1Telemetry` | 148 | 868 |
| `barcode_service_t` | 488 | 860 |
| `TerrainSnapshot` | 40 | 336 |

`ObstacleSnapshot`, `MotionFeedback` and `MotionCommand` were not found under `app_program/` in `main` and are not reported.

## Task stacks

`TASK_STACK_SIZE` is 2048 bytes (`app_program/app.h`), set as `ctsk.stksz` in `app_main.c` for the initial task. Stack bytes = task count × 2048. Application task creation sites:

- `main`: 4 (Motion, terrain_task, IR line, Line follow)
- `uat`: 7 (Motion, Obstacle, Terrain, IR line, Safety, Mission, Reporting)

Live task counts at runtime were not recorded.

## Not measured

- Loop timing for each periodic task. Needs an on-target run.
- MQTT and telemetry payload sizes. Needs an on-target run.
- Per-object `nm` sizes.
- No firmware was executed or flashed. These are compile and link results only.

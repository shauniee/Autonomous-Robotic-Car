# CR baseline (before): main and uat

Issue #22 (`test/cr-baseline-before`). Re-measured, not copied from the earlier uat census.

## Sources and toolchain

| Build | Commit | Source | Makefile used |
| --- | --- | --- | --- |
| A `main` | `c6c4ef9` | pinned commit | overlay of `build_make/makefile` and `build_make/pico_rp2040.mk` from `chore/build-hygiene` (821b442) |
| B `uat` | `d8220bc` | pinned commit | its own `build_make` (not overlaid; see Flags) |
| C `main` + issue #21 | `821b442` (branch `test/cr-baseline-before`) | pinned commit | the same as A |

Toolchain: Arm GNU Toolchain 15.3.Rel1, `arm-none-eabi-gcc 15.3.1 20260627`. Host: macOS 25.5.0, GNU make 3.81.

Build command, run in each tree's `build_make/`:

```sh
make clean
make -k -j8 SMP=1 CONSOLE=uart WIFI=none NET=none MQTT=0 CRASH_REPORT=0 TEST=1
```

Only the UART profile was built. The `usb_cdc` profile needs the Pico SDK, which is not present at a usable path on this machine, so it was not built.

## Flags

- A and C use the flags from issue #21: `-Wall -Wextra -Wno-unused-parameter -Wno-cast-function-type -ffunction-sections -fdata-sections`, and `-Wl,--gc-sections` on the link.
- B uses its own makefile. Overlaying the #21 makefile onto `uat` fails to link with `undefined reference to subsystemN_test` and `integration_test`, because `uat` pulls its test fixtures through its own `testing/firmware.mk`. The two builds therefore do not share identical flags. Only A and C share the same flag set.
- `main` at `c6c4ef9` links only with the #21 link flags: its own makefile fails with `undefined reference to _write, _read, _close, _lseek, _sbrk`.

## Section sizes

Command: `arm-none-eabi-size mtk3pico_smp1_uart_test.elf`

| Build | text | data | bss |
| --- | ---: | ---: | ---: |
| A `main` (#21 flags) | 63212 | 536 | 19092 |
| C `main` + #21 branch | 63212 | 536 | 19092 |
| B `uat` (own makefile) | 134004 | 536 | 24616 |

A and C produce identical section sizes; the #21 warning fixes do not change code size. Per-object `nm` sizes were not recorded in this pass.

## CR-scope struct sizes

Command: a probe file with `const unsigned long sizeof_X = sizeof(X);`, compiled with `arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -ffreestanding -std=gnu11 -O2 -DCNF_SMP=1 -DAPP_TEST=1 -DTM_CONSOLE_UART=0 -DBOARD_UART0_FREE=1 -D_PICO_RP2040_ -I include -I config -I kernel/knlinc -I . -S`, then the `.word` value read from the `.s` output. Measured on build C (`821b442`, same as A).

| Struct | Source | Size (bytes) |
| --- | --- | ---: |
| `S1Telemetry` | `app_program/subsystem1/telemetry.h` | 148 |
| `barcode_service_t` | `app_program/subsystem3/barcode_service.h` | 488 |
| `TerrainSnapshot` | `app_program/subsystem4/subsystem4.h` | 40 |

`ObstacleSnapshot`, `MotionFeedback` and `MotionCommand` were not found under `app_program/` at this commit and are not reported.

## Task stacks

Stacks are kernel-allocated. Stack size is `TASK_STACK_SIZE` = 2048 bytes (`app_program/app.h`) for the initial task created in `app_main.c`. Application tasks created through `app_start_task` in build C: 4 call sites (Motion, IR line, terrain_task, Line follow), all using the same default. Stack bytes total = task count × 2048 bytes. The live task count at runtime was not recorded.

## MQTT and telemetry payload sizes

Not measured. Capturing payload bytes needs the on-target MQTT stream, which this pass did not run.

## Loop timing

Not measured. Loop timing needs on-target runs with the TIMER, and this pass did not run firmware on hardware.

## Not run

- No firmware was executed or flashed. The builds are compile and link evidence only.
- No physical or on-target result is implied.

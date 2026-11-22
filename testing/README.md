# Testing

## Contents

- [Test folders](#test-folders)
- [Run the test code](#run-the-test-code)
- [Run and record a hardware test](#run-and-record-a-hardware-test)

## Test folders

Each folder contains a runbook, a `TEST_LOG.md` with the test cases and observed results, and a `logs/<test ID>/` directory for captures and any host test code for that ID.

| Scope | Test IDs | Runbook |
| --- | --- | --- |
| Communications | S1-U01–U05 | [Comms](comms/README.md) |
| Motion | S2-U01–U05 | [Motion](motion/README.md) |
| Line and barcode | S3-U01–U05 | [Line and barcode](line_barcode/README.md) |
| Terrain | S4-U01–U03 | [Terrain](terrain/README.md) |
| Obstacle | S5-U01–U03 | [Obstacle](obstacle/README.md) |
| Complete robot on the course | SYS-U01 | [System](system/README.md) |

## Run the test code

From the repository root:

```sh
sh testing/tools/run.sh           # all host tests
sh testing/tools/run.sh S2-U05    # code associated with one ID
sh testing/tools/run.sh tools     # checks of the laptop tools
```

Host tests exercise the subsystem code with simulated inputs. Firmware checks live in `app_program/<subsystem>/<subsystem>_test.c`. Boot the motor-inhibited **test** image on USB to see the on-board results as `[TEST] <ID> PASS|FAIL`. A manual test has its setup and expected observations in the subsystem runbook.

## Run and record a hardware test

Choose the image in the runbook and follow the [build and flashing guide](../README.md#build-profiles). Record its commit and profile. Capture the USB console for a USB-only test or the MQTT monitor for a battery run. Save the capture under the test ID and append the observed result, measurements and capture filename to the corresponding test log.

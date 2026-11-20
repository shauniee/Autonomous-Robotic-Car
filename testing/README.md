# Testing

## Contents

- [Test folders](#test-folders)
- [Run the test code](#run-the-test-code)
- [Run and record a hardware test](#run-and-record-a-hardware-test)
- [Tribunal trial plan](#tribunal-trial-plan)

## Test folders

Each folder contains a runbook, a `TEST_LOG.md` with the test cases and observed results, and a `logs/<test ID>/` directory for captures and any host test code for that ID.

| Scope | Test IDs | Runbook |
| --- | --- | --- |
| Communications | S1-U01–U05 | [Comms](comms/README.md) |
| Motion | S2-U01–U05 | [Motion](motion/README.md) |
| Line and barcode | S3-U01–U05 | [Line and barcode](line_barcode/README.md) |
| Terrain | S4-U01–U04 | [Terrain](terrain/README.md) |
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

## Tribunal trial plan

The table maps each trial to the test that exercises it.

| ID | Trial | Tested in |
| --- | --- | --- |
| T01 | Electrical release: markings, continuity, loaded rails/current, signal highs, shared contacts and jumper | none |
| T02 | Boot/stop/reset in every motion mode; output-brake timing and physical stopping separately | S4-U04, SYS-U01 |
| T03 | Straight/curve/junction tracking under lighting, surface and speed variation | SYS-U01 |
| T04 | Known barcode scales/speeds in both directions, same-character decoding, pending-to-junction execution, duplicate/partial rejection and capture overflow | S3-U03 to S3-U05, SYS-U01 |
| T05 | Four encoder actions: repeated distance/angle accuracy, completion, timeout and obstacle cancellation | S2-U02, S2-U03 |
| T06 | Labelled stationary/acceleration/turn/climb/descent/lift traces; no raw gyro claim | none |
| T07 | Measured reference humps, slip/dynamic/incomplete rejection and invalid estimate after valid maximum | S4-U03 |
| T08 | Known forward ranges/materials/angles; no/late/out-of-range echo, settled repeated filtering and repeat timing | S5-U01 |
| T09 | Moving scan rejection, coarse/fine profile for 100-200 mm widths, uncertainty, separate front age and restored-center acquisition | S5-U03 |
| T10 | Vehicle envelope/course boundary, both bypass sides, blocked paths and original-line versus wrong-crossing recovery | SYS-U01 |
| T11 | Controlled line loss, interrupted search, obstacle during search and bounded timeout | none |
| T12 | Actual mission telemetry/status subscription, units/validity and rejection of prohibited/malformed commands | S1-U04 |
| T13 | WiFi/broker loss/reconnect under sensor/scan load; healthy local control and no stale replay | S1-U03, SYS-U01 |
| T14 | Stale/frozen producers, encoder/stall faults, expired command and full diagnostic/navigation/capture buffers | none |
| T15 | Concurrent load: releases/WCET, missed deadlines, stop latency, queue occupancy and stack/memory usage | none |
| T16 | Repeated looping-course rehearsal with junction actions, stationary U-turn pause, humps, bypass, telemetry and local-stop ending | SYS-U01 |

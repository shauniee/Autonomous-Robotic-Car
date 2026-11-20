# Testing

## Contents

- [Layout](#layout)
- [Three levels of test](#three-levels-of-test)
- [Run the test code](#run-the-test-code)
- [How to run and record a test](#how-to-run-and-record-a-test)
- [Videos of tests and demos](#videos-of-tests-and-demos)
- [Rules](#rules)
- [Continuous integration](#continuous-integration)
- [Tribunal trial plan](#tribunal-trial-plan)

All test records for the project live in this folder. A test only counts as
done when it is **written down here with its result and a saved log**.

## Layout

```
testing/
├── README.md              this guide
├── comms/                 S1, kept by its owner
│   ├── README.md          runbook: how to run each test and the commands
│   ├── TEST_LOG.md        test cases + results
│   └── logs/<test ID>/    one folder per test, e.g. logs/S1-U01/, holding
│                          its <what>.log files (usb.log, monitor.log) and the host test
│                          code for that ID (test_*.c, .py or .mjs)
├── motion/                S2
├── line_barcode/          S3
├── terrain/               S4
├── obstacle/              S5
├── integration/           tests across two or more subsystems (shared)
├── system/                tests of the whole robot against its requirements
│                          (shared), plus the mission-readiness checklist
└── tools/                 the host-test runner, firmware build and size probe,
                           and checks of the repo scripts (removed before the
                           final submission)
```

Choose the folder by what the test exercises, not where it ran. A barcode read
tests line_barcode only. Line following, junction turns, bypass and a hump while
following need Mission to combine subsystems, so they are integration.

## Three levels of test

| Level | ID format | Tests what | Written by | Where |
|---|---|---|---|---|
| Unit | `S1-U01` to `S5-U05`, up to five per subsystem | one subsystem on its own | the subsystem owner | `<subsystem>/TEST_LOG.md` |
| Integration | `INT-01` to `INT-10` | subsystems working together through Mission | the owners on both sides | `integration/TEST_LOG.md` |
| System | `SYS-01` to `SYS-10` | the finished robot against a requirement | whole team | `system/TEST_LOG.md` |

Test IDs never change and are never reused, even if a test is dropped. On
2026-11-09 the earlier tests and logs were removed and these IDs restarted;
code and docs cite that older evidence by date, and it stays in git history
(commit `67bfa3b` and earlier).

## Run the test code

Each subsystem's `README.md` is its runbook: what to set up, the steps for every
test ID and the exact commands. Host test code (the real subsystem code with
fake hardware, no robot needed) sits in the folder of the test ID it backs:

```sh
sh testing/tools/run.sh            # every host test, as CI runs it
sh testing/tools/run.sh S2-U05     # only the code in */logs/S2-U05/
sh testing/tools/run.sh tools      # only the checks in testing/tools/
```

A test ID with no code (a manual test) prints `no tests for <ID>`. The on-car
automatic checks are in `app_program/<subsystem>/<subsystem>_test.c` and run on
`./flash.sh test`, which prints `[TEST] <ID> PASS|FAIL`.

## How to run and record a test

1. **Define it first.** Add a row to the *Test cases* table in the right
   `TEST_LOG.md`: ID, what it checks, how, and the expected result.
2. **Automate it if you can.** Add a `TEST_CHECK` with the same ID to
   `app_program/<subsystem>/<subsystem>_test.c` (see `app_program/test_report.h`).
   Host test code goes in `logs/<test ID>/` and gets one line in
   `testing/tools/run.sh`. A test that needs eyes or instruments (a motor turns, a sensor reacts, a
   tape measure) is a *manual* test. Write its steps in the table instead.
3. **Build the image and flash it** ([build and flash](../README.md#getting-started)).
   `./flash.sh test` runs every subsystem's checks at power-up, prints one
   `[TEST] <ID> PASS|FAIL` line per check and never drives the motors.
   `./flash.sh trial` is the full course with diagnostics; follow the
   [power policy](../README.md#power-and-flashing).
4. **Save the console output** to `logs/<test ID>/`, named `<what>.log`
   (for example `system/logs/SYS-04/usb.log`). Logs are plain text
   `.log` files only:
   ```sh
   # macOS (find the port name with: ls /dev/cu.usbmodem*)
   mkdir -p testing/system/logs/SYS-04
   cat /dev/cu.usbmodemMTK3PICO1 | tee testing/system/logs/SYS-04/usb.log
   # laptop monitor during a battery run
   ~/.robot-car-tools/venv/bin/python scripts/mqtt_client.py monitor --broker 127.0.0.1 | tee testing/system/logs/SYS-04/monitor.log
   ```
   Start the command first, then reset the Pico. Press Ctrl-C when it is done.
   On Windows with PuTTY, use *Session → Logging → All session output*.
   Photos and scope screenshots are not kept here: show them in the video.
   Videos are not saved here either.
   Do not save build or host-test output.
5. **Record the result.** Add a row to the *Results* table: date, test ID,
   the commit you tested (`git rev-parse --short HEAD`, with `+` for
   uncommitted changes), the subsystem owner as tester, PASS/FAIL, the log file and the video
   link. Commit the log and the table together. A build or host test is not
   a car pass.

## Videos of tests and demos

Record 15–60 s showing the test ID, the commit and the hardware; a full course
can run as long as the run. Share it as an **unlisted YouTube** link, list it
in [video/README.md](../video/README.md), and put the link in the
*Log / evidence* column of the test's result row.

## Rules

- **Never edit or delete an old result.** If a test fails, record the FAIL,
  fix the code, re-run, and add a new PASS row. The history is the evidence.
- **Always record the commit.** A result is meaningless without the exact
  code it was run on. Commit your work *before* testing.
- **A FAIL needs a note** saying what went wrong and, later, which commit fixed it.
- Before merging a change into `main`, re-run your subsystem's unit tests.
  Before a demo or hand-in, run every test and record the full set.

## Continuous integration

`.github/workflows/host-tests.yml` runs on every pull request and every push to `main`. None of it runs on the car.

| Job | Command | Fails when |
| --- | --- | --- |
| Coding standard | `python3 scripts/check_barr.py --all` | Any `app_program` line breaks a mechanical BARR-C:2018 rule |
| Deliverables | `python3 scripts/check_deliverables.py` | A subsystem's source, public function, wired-in test or report section is missing; a second motor writer appears; networking commands Motion; firmware and laptop frame lengths differ |
| Host tests | `bash testing/tools/run.sh` (includes the Node dashboard check) | Any test of the real subsystem code fails |
| Firmware build | `sh testing/tools/build_pending_profiles.sh` | Any of the six images fails to compile or link, or a struct outgrows its bound in `testing/tools/sizeof_probe.c`. It never flashes |

Deliverables that need the car print NEEDS EVIDENCE and do not fail the check, because only a physical run can close them.

## Tribunal trial plan

Which test now covers each Tribunal trial. Limits come from
[pending decisions](../docs/project-specs.md#pending-decisions), not from a test name.

| ID | Trial | Tested in |
| --- | --- | --- |
| T01 | Electrical release: markings, continuity, loaded rails/current, signal highs, shared contacts and jumper | none (SYS-01 dropped) |
| T02 | Boot/stop/reset in every motion mode; output-brake timing and physical stopping separately | SYS-06 |
| T03 | Straight/curve/junction tracking under lighting, surface and speed variation | none (INT-01 and INT-05 dropped) |
| T04 | Known barcode scales/speeds in both directions, same-character decoding, pending-to-junction execution, duplicate/partial rejection and capture overflow | S3-U03 to S3-U05, INT-03 |
| T05 | Four encoder actions: repeated distance/angle accuracy, completion, timeout and obstacle cancellation | S2-U02, S2-U03 |
| T06 | Labelled stationary/acceleration/turn/climb/descent/lift traces; no raw gyro claim | none (S4-U02 dropped 2026-11-18) |
| T07 | Measured reference humps, slip/dynamic/incomplete rejection and invalid estimate after valid maximum | S4-U03 |
| T08 | Known forward ranges/materials/angles; no/late/out-of-range echo, settled repeated filtering and repeat timing | S5-U01 |
| T09 | Moving scan rejection, coarse/fine profile for 100-200 mm widths, uncertainty, separate front age and restored-center acquisition | S5-U03 |
| T10 | Vehicle envelope/course boundary, both bypass sides, blocked paths and original-line versus wrong-crossing recovery | INT-09 |
| T11 | Controlled line loss, interrupted search, obstacle during search and bounded timeout | none (INT-02 dropped) |
| T12 | Actual mission telemetry/status subscription, units/validity and rejection of prohibited/malformed commands | S1-U04 |
| T13 | WiFi/broker loss/reconnect under sensor/scan load; healthy local control and no stale replay | S1-U03, SYS-08 |
| T14 | Stale/frozen producers, encoder/stall faults, expired command and full diagnostic/navigation/capture buffers | none (SYS-07 dropped) |
| T15 | Concurrent load: releases/WCET, missed deadlines, stop latency, queue occupancy and stack/memory usage | none (SYS-10 dropped) |
| T16 | Repeated looping-course rehearsal with junction actions, stationary U-turn pause, humps, bypass, telemetry and local-stop ending | SYS-04, SYS-09 |

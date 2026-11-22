# Testing

## Contents

- [Layout](#layout)
- [Two levels of test](#two-levels-of-test)
- [Run the test code](#run-the-test-code)
- [How to run and record a test](#how-to-run-and-record-a-test)
- [Videos of tests and demos](#videos-of-tests-and-demos)
- [Rules](#rules)
- [Continuous integration](#continuous-integration)

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
├── system/                SYS-U01, the full course run of the whole robot (shared)
└── tools/                 the host-test runner, firmware build and size probe,
                           and checks of the repo scripts (removed before the
                           final submission)
```

Choose the folder by what the test exercises, not where it ran. A barcode read
tests line_barcode only. Line following, junction turns, bypass and a hump while
following need Mission to combine subsystems, so they are tested in the full
course run (SYS-U01).

## Two levels of test

| Level | ID format | Tests what | Written by | Where |
|---|---|---|---|---|
| Unit | `S1-U01` to `S5-U05`, up to five per subsystem | one subsystem on its own | the subsystem owner | `<subsystem>/TEST_LOG.md` |
| System | `SYS-U01`, one full course run | the finished robot on the course | whole team | `system/TEST_LOG.md` |

Test IDs never change and are never reused, even if a test is dropped.
Earlier tests and logs were removed when these IDs restarted; they stay in
git history (commit `67bfa3b` and earlier).

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
   (for example `motion/logs/S2-U01/usb.log`). Logs are plain text
   `.log` files only:
   ```sh
   # macOS (find the port name with: ls /dev/cu.usbmodem*)
   mkdir -p testing/motion/logs/S2-U01
   cat /dev/cu.usbmodemMTK3PICO1 | tee testing/motion/logs/S2-U01/usb.log
   # laptop monitor during a battery run
   ~/.robot-car-tools/venv/bin/python scripts/mqtt_client.py monitor --broker 127.0.0.1 | tee testing/system/logs/SYS-U01/monitor.log
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

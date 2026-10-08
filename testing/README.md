# Testing

All test records for the project live in this folder. A test only counts as
done when it is **written down here with its result and a saved log**.

```
testing/
├── README.md              this guide
├── subsystem1/            one folder per subsystem, kept by its owner
│   ├── TEST_LOG.md        test cases + results
│   └── logs/              saved console output, photos, scope captures
├── ...
├── subsystem5/
├── integration/           tests across two or more subsystems (shared)
└── system/                tests of the whole product against its requirements (shared)
```

## Three levels of test

| Level | ID format | Tests what | Written by | Where |
|---|---|---|---|---|
| Unit | `S1-U01`, `S2-U01`, … | one subsystem on its own | the subsystem owner | `subsystemN/TEST_LOG.md` |
| Integration | `INT-01`, … | an interface between subsystems (TEAM.md section 5) | the owners on both sides | `integration/TEST_LOG.md` |
| System | `SYS-01`, … | the finished product against a requirement | whole team | `system/TEST_LOG.md` |

Test IDs never change and are never reused, even if a test is dropped.

## How to run and record a test

1. **Define it first.** Add a row to the *Test cases* table in the right
   `TEST_LOG.md`: ID, what it checks, how, and the expected result.
2. **Automate it if you can.** Add a `TEST_CHECK` with the same ID to
   `app_program/subsystemN/subsystemN_test.c` (see `app_program/test_report.h`).
   A test that needs eyes or instruments (an LED blinks, a motor turns, a
   scope shows 1 kHz) is a *manual* test. Write its steps in the table instead.
3. **Build the test image and flash it.**
   ```sh
   cd build_make
   make TEST=1 -j8        # -> mtk3pico_smp1_usb_cdc_wifi_test.uf2
   ```
   The test image runs every subsystem's tests at power-up and prints one
   `[TEST] <ID> PASS|FAIL` line per check. (A normal `make` leaves the tests out.)
4. **Save the console output** to `logs/`, named `YYYY-MM-DD_<what>.txt`:
   ```sh
   # macOS (find the port name with: ls /dev/cu.usbmodem*)
   cat /dev/cu.usbmodem1101 | tee ../testing/subsystem1/logs/2026-10-14_unit.txt
   # Linux / WSL with usbipd
   cat /dev/ttyACM0 | tee ../testing/subsystem1/logs/2026-10-14_unit.txt
   ```
   Start the command first, then reset the Pico. Press Ctrl-C when it is done.
   On Windows with PuTTY, use *Session → Logging → All session output*.
   For manual tests, save photos or scope screenshots in `logs/` the same way
   (keep each file under a few MB). Videos go in `videos/` instead (see below).
5. **Record the result.** Add a row to the *Results* table: date, test ID,
   the commit you tested (`git rev-parse --short HEAD`), who ran it,
   PASS/FAIL, and the log file. Commit the log and the table together.

## Videos of tests and demos

A manual test (an LED, a motor, a sensor reacting) is best shown with a short
video. Record 15–60 s showing the test ID and the hardware, share it as an
**unlisted YouTube** link (the only way videos are shared), list it in
**`videos/README.md`**, and put the link in the *Log / evidence* column of
the test's result row. `videos/README.md` explains how.

## Rules

- **Never edit or delete an old result.** If a test fails, record the FAIL,
  fix the code, re-run, and add a new PASS row. The history is the evidence.
- **Always record the commit.** A result is meaningless without the exact
  code it was run on. Commit your work *before* testing.
- **A FAIL needs a note** saying what went wrong and, later, which commit fixed it.
- Before merging a change into `main`, re-run your subsystem's unit tests.
  Before a demo or hand-in, run every test and record the full set.

## S3 proportional bench workflow

The [S3 guide](../docs/architecture/LINE_FOLLOWING.md) defines the current states and recovery policy; [future PID/barcode work](../docs/architecture/IR_FUTURE_PLANS.md) is deferred. Run sh scripts/check-ir-tracking.sh from code/ for host calculation/controller, fake ADC, real S3 loop with fake inputs/S2, and unchanged real S2 service with a fake kernel. Host checks do not establish physical GPIO/scheduler/motor performance. Record host and on-target runs separately even when they share a test ID.

S3-U03 belongs to the earlier PID bench and is retired after this override; keep its definition/results. S3-U06/U07/U08 cover the new P, capture/classification and recovery policies; INT-05 exercises the real S3 loop. TEST=1 contains S3-U02 and S3-U06 self-checks, but a compiled image is not an on-target PASS.

Use the exact normal/TEST build options in [getting started](../docs/GETTINGS_STARTED.md). USB alone is used for inhibited TEST logging; disconnect USB before powering battery trials and recalibrate after reboot. Use camera/scope/operator evidence for battery trials, never simultaneous USB/power-bank logging.

The saved 2026-10-08 migration and proportional-control checks ran against an uncommitted working tree. Their SHA256 source manifests plus base HEAD identify the tested source; the base HEAD alone does not. Preserve original evidence and give later runs distinct filenames. Formal hardware trials record a committed source revision and flashed-image checksum.

## Wireless telemetry workflow

Run the same host script for S1-U04/U05, INT-06 and INT-07 as well as the existing S3/S2 cases. It exercises the actual store/encoder/adapter and real S3 loop with fake kernel/hardware/network calls; it does not connect a Pico. With Mosquitto installed, `python3 tests/test_mqtt_broker.py` separately runs INT-08 against a temporary **loopback-only** broker, then shuts it down. Its synthetic fixture cannot count as robot telemetry.

Use [getting started section 5](../docs/GETTINGS_STARTED.md#5-wireless-telemetry-on-the-mac) for explicit normal/TEST MQTT build flags and the offline profile. INT-09 records those builds. TEST firmware includes S1-U04 without networking in the self-check; a built image is not an on-target PASS.

Actual Pico connectivity is SYS-09, followed by bank-only trials SYS-10. Keep USB alone for inhibited console checks and disconnect it before powering from the bank. Save `mosquitto_sub -h 127.0.0.1 -p 1883 -t robot/s3/telemetry -v` output for wireless observations. Record source revision, image checksum, wiring/power source and network-test actions. The 500 ms stream cannot prove a 100 ms recovery deadline; camera/scope records are still needed.

The distinct `2026-10-08_s3-mqtt-*` logs/manifests record this branch's precommit working-tree software checks. Historical logs remain intact. S1 snapshot failure and broker loss are diagnostics failures; they must never be counted as sensor progress or used to inhibit healthy local motion.

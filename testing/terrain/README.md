# Terrain (S4) test runbook

## Contents

- [Before you start](#before-you-start)
- [Which image to flash](#which-image-to-flash)
- [Captures](#captures)
- [Run the test code](#run-the-test-code)
- [Every video](#every-video)
- [After each test](#after-each-test)
- [Tests](#tests)
  - [S4-U01](#s4-u01-imu-boot-calibration)
  - [S4-U02](#s4-u02-hump-count-and-height)
  - [S4-U03](#s4-u03-mission-stop-on-lift-and-flip)

How to run and record S4-U01, S4-U02 and S4-U03. See [TEST_LOG.md](TEST_LOG.md) for test definitions and prior results.

## Before you start

1. Work on a fresh branch from `main`: `git fetch` then `git switch -c test/s4-terrain origin/main`.
2. Commit nothing else while testing; the image is built from the working tree, so `git status` must be clean. Note the commit: `git rev-parse --short HEAD`.
3. Follow the [power policy](../../README.md#power-and-flashing): USB alone for flashing and TEST images; battery alone for driving.
4. For S4-U03, start the broker if it is not already running:

   ```bash
   "$(brew --prefix)/sbin/mosquitto" -c config/mosquitto-lab.conf
   ```

## Which image to flash

Host test code (`sh testing/tools/run.sh <ID>`) runs on the laptop and needs no flash. Use the image named in each test's **Image** line and finish any tests that can use the current image before flashing again.

| Image | Power | Tests |
| --- | --- | --- |
| `TERRAIN_DIAGNOSTICS=1 ./flash.sh test` | USB only | S4-U01, S4-U02 |
| `./flash.sh trial` | Battery only | S4-U03 |

Before every flash:

1. For S4-U03, `ipconfig getifaddr en0` must print the `MQTT_BROKER_IP` in `config/wifi_secrets.h`. If the hotspot gave the laptop a new address, update that line before flashing so the Pico can reach the broker.
2. Check that `git status` is clean and record the exact firmware commit used for the run.

## Captures

Keep the latest capture for each test under `testing/terrain/logs/<test ID>/`. Start the capture **before** resetting the Pico and stop it with Ctrl-C when the test ends.

USB console for S4-U01:

```bash
cat /dev/cu.usbmodemMTK3PICO1 | tee testing/terrain/logs/S4-U01/usb.log
```

S4-U02 and S4-U03 use the capture commands in their test steps below.

## Run the test code

Host test code for a test ID is in `logs/<ID>/` beside its logs. It runs the real subsystem code with fake hardware, so it needs no robot. Run it before flashing:

```bash
sh testing/tools/run.sh <ID>   # one test ID, for example the first one below
sh testing/tools/run.sh        # every host test, as CI runs it
```

A manual test with no code prints `no tests for <ID>`. The on-car check for S4-U01 is in `app_program/terrain/terrain_test.c`; `./flash.sh test` runs it at power-up and prints `[TEST] <ID> PASS|FAIL`.

## Every video

1. Hold up or say the test ID.
2. Show `git rev-parse --short HEAD` in a terminal.
3. Show the hardware and the capture terminal while the test runs.
4. End on the result (console line, dashboard or tape measure).
5. Upload to YouTube as **Unlisted** and add it to the [video index](../../video/README.md).

## After each test

Give the subsystem owner the test ID, the video link and anything you measured (tape readings, counts). The owner checks the capture and commits it with the updated result row in [TEST_LOG.md](TEST_LOG.md) using `test(terrain): <ID> <PASS|FAIL|PARTIAL>`.

## Tests

### S4-U01 IMU boot calibration

**Image:** `TERRAIN_DIAGNOSTICS=1 ./flash.sh test`

**Set up:** USB capture started.

**Steps:**

1. Boot with the car still: `[IMU] calibration passed`.
2. Boot while rocking the car: `[IMU] calibration failed`.
3. Power off, unplug the IMU, boot: calibration fails; press GP20 and the start is refused.

**Show on video:** Each `[IMU]` line, the refused start and, on the still boot, the self-check line `[TEST] S4-U01 PASS`.

**Test code:** `app_program/terrain/terrain_test.c` (on the car)

**Run:** flash the image above, start the captures and follow the steps.

**Logs:** `usb.log`

### S4-U02 Hump count and height

**Image:** `TERRAIN_DIAGNOSTICS=1 ./flash.sh test` (USB only).

**Set up:** Measure the hump to confirm it is 10 mm. In macOS Terminal (zsh), run this from the repo root before resetting the Pico. It reopens USB after a reset, saves the full console output, and displays calibration and completed hump estimates for the video. The display applies `HUMP_CALIB_FACTOR` from the configuration used to build the TEST image to each valid rise sample.

```zsh
(
  while true; do
    for port in /dev/cu.usbmodemMTK3PICO*(N); do
      cat "$port"
    done
    sleep 0.1
  done
) | tee testing/terrain/logs/S4-U02/usb.log | awk '
  $1 == "#define" && $2 == "HUMP_CALIB_FACTOR" { scale = $3 + 0 }
  /^\[IMU\] calibration / {
    calibrated = ($0 ~ /calibration passed/)
    count = 0
    print
    fflush()
  }
  /^\[IMU_RAW\] / && split($0, imu, ",") == 17 &&
      imu[15] ~ /^[0-9]+$/ && imu[16] ~ /^[0-9]+$/ {
    if (imu[9] == "0") calibrated = 0
    if (imu[9] == "1" && !calibrated) {
      calibrated = 1
      print "[IMU] calibrated (confirmed by USB samples)"
      fflush()
    }
    humps = imu[15] + 0
    if (humps < count) count = humps
    if (imu[14] == "1" && humps > count && scale > 0) {
      count = humps
      printf "humps=%d | height estimate=%.1f mm\n", humps, imu[16] * scale / 10
      fflush()
    }
  }
' app_program/common/robot_config.h -
```

**Steps:**

1. Reset the car and keep it still on level ground until the terminal shows `[IMU] calibration passed` or `[IMU] calibrated (confirmed by USB samples)`. The TEST image keeps the motors inhibited.
2. Push the car forward by hand so one wheel crosses the hump. Let it settle on flat ground and wait for the new `humps=` line with its height estimate.
3. Return to the starting side on flat ground and repeat for three crossings total. Press Ctrl-C to stop the capture.

**Show on video:** The ruler at 10 mm, each crossing, and the USB terminal showing `humps=1`, `humps=2`, `humps=3` and each corresponding height estimate. Record the three estimates against the 10 mm reference. The height error tolerance is pending under [D10](../../docs/project-specs.md#pending-decisions).

**Test code:** `logs/S4-U02/test_terrain.c` (host).

**Run:** `sh testing/tools/run.sh S4-U02`, then flash the image above and follow the capture steps.

**Logs:** `usb.log`

### S4-U03 Mission stop on lift and flip

**Image:** `./flash.sh trial`. Flash over USB with the battery disconnected. Unplug USB before connecting the battery for the trial.

**Set up:** With the broker running, start the capture from the repo root. Then boot on battery with the car still on the floor, wait for `state=READY | Terrain: none`, and leave GP20 untouched. The terminal shows changes in mission state or terrain faults. The log keeps the full capture.

```bash
~/.robot-car-tools/venv/bin/python -u scripts/mqtt_client.py monitor --raw --broker 127.0.0.1 \
  | perl -MTime::HiRes=time -ne 'BEGIN { $| = 1 } printf "%.3f %s", time, $_' \
  | tee testing/terrain/logs/S4-U03/monitor.log \
  | perl -ne '
      BEGIN { $| = 1 }
      print STDERR $_ if /^\S+ (?:Error:|Broker refused connection:)/;
      $previous = "" if /^\S+ STATUS /;
      next unless /^\S+ TELEMETRY \S+ (state=\S+).* \| faults: (.*)$/;
      my ($state, $faults) = ($1, $2);
      my @terrain = grep { /^Terrain: / } split /, /, $faults;
      my $line = "$state | " . (@terrain ? join(", ", @terrain) : "Terrain: none");
      next if $line eq $previous;
      print "$line\n";
      $previous = $line;
    '
```

**Steps:**

1. Lift the car straight up until the monitor shows `Terrain: lifted` and `state=STOPPED`, then set the car down flat.
2. Wait for the fault to clear and show that `state=STOPPED` persists. Press and release GP21 and show `state=READY`.
3. Turn the car over until the monitor shows `Terrain: lifted` and `state=STOPPED`, then set it back on its wheels.
4. Wait for the fault to clear. Press and release GP21 and show `state=READY`. Press Ctrl-C to stop the capture.

**Show on video:** Each lift and flip, the `Terrain: lifted` fault and `state=STOPPED` in the terminal, and `state=READY` after each GP21 press.

**Test code:** none (manual test)

**Run:** Flash the image above, start the capture and follow the steps.

**Logs:** `monitor.log`

# Phase 1: IR-to-Motion development handoff

Recorded handoff status: software template available; physical motor operation is not verified and the owner reported that motors did not work. This handoff preserves the existing runtime. Motor fixes, stop serialization and reset acknowledgement belong to later approved phases.

## Where Nicholas should develop

The running path already exists:

```text
S3 drv_ir -> ir_tracking -> robot_control -> task_ir
   -> s2_publish(MotionRequest) -> S2 motion_task -> drv_motor
```

| File | Responsibility |
| --- | --- |
| [drv_ir.c](../../app_program/subsystem3/drv_ir.c) | Paired GP26/ADC0 left and GP27/ADC1 right acquisition |
| [ir_tracking.c](../../app_program/subsystem3/ir_tracking.c) | Black/white calibration, line classification and proportional steering |
| [robot_control.c](../../app_program/subsystem3/robot_control.c) | READY/FOLLOW_LINE/RECOVER_LINE/STOPPED, duty mixing and local start/reset policy |
| [task_ir.c](../../app_program/subsystem3/task_ir.c) | Existing single S3 task: buttons, acquisition, publishing, feedback and watchdog |
| [subsystem2.h](../../app_program/subsystem2/subsystem2.h) | Public Motion interface; include this when calling S2 |
| [motion_policy.h](../../app_program/subsystem2/motion_policy.h) | Current request type and limits |
| [subsystem2.c](../../app_program/subsystem2/subsystem2.c) | Exclusive motor service, stop latch and request expiry |

Start with the existing [line-tracking catalogue](LINE_TRACKING.md) and preserved firmware self-checks. Keep one S3 producer and one S2 motor writer; do not add a second motor task or write PWM from the IR subsystem. The application initializes subsystem objects before starting tasks. The template already runs micro T-Kernel; no separate kernel installation into an application image is required.

## Current interface contract

| Item | Contract |
| --- | --- |
| `MotionRequest.left`, `.right` | Forward PWM fractions, finite and in inclusive range 0.0 to 0.35. 0.10 means 10% duty, not a measured speed. Reverse and encoder control are unavailable. |
| `.sampled_ms` | Original paired acquisition time, in the same monotonic millisecond clock used by `tk_get_otm().lo`. Preserve it while publishing a cached sample. |
| `.enabled` | True only when the S3 state permits motion. False requests braking; it does not by itself set the persistent stop latch. |
| `s2_publish(request)` | Latest-value mailbox with a mutex, not a queued action. Check `ER < E_OK`. Success means the request was copied, not that a motor moved. S2 validates on service. |
| Expiry | Unsigned age <=60 ms is accepted; age >60 ms is rejected. For a sample at 100 ms, service at 160 ms may accept it and at 161 ms must reject it. An enabled invalid/expired request brakes and latches stop. |
| `s2_stop()` | Sets a persistent event flag and independently wakes S2, without acquiring the request mutex. S2 boots with this latch set. |
| `s2_reset_stop()` | Clears the latch and discards the previous request. Use only through the existing fresh local start/reset transitions. Do not call it every sample or automatically after a fault. |
| `s2_health(&health)` | Software service progress: sequence and cycle-entry `serviced_ms`, plus the current stop latch. Check its return value before consuming it. It does not prove successful PWM writes, measured speed or physical stationarity. |

S3 services inputs every 10 ms and paired IR acquisition every 20 ms. S2 normally services every 20 ms and can wake for stop. Preserve the acquisition timestamp even when S3 republishes between samples; using publication time would hide stale sensing. Invalid acquisition must not become an enabled request with refreshed time.

This is an equivalent example of the publication already in `task_ir.c`, using its existing `RobotControl car`. It is a replacement reference for that block, not an additional loop/task:

```c
MotionRequest request = {
    .left = car.left_duty,
    .right = car.right_duty,
    .sampled_ms = car.tracking.sample_ms,
    .enabled = robot_control_is_moving(&car)
};

if (s2_publish(request) < E_OK)
{
    robot_control_stop(&car, "publish_failed");
    s2_stop();
}
```

Preserve the existing health, stop and watchdog handling around this block. S3 owns the line-position controller; S2 currently applies its bounded duties. Later wheel-speed PID will regulate measured wheel speeds without requiring a replacement of S3's proportional line controller.

## Reproducible software builds

Run from the repository root `Project/Autonomous-Robotic-Car`, with `arm-none-eabi-gcc` on PATH. The full Pico SDK must include TinyUSB and, for the radio profile, CYW43 dependencies. The current make logic needs an SDK path without spaces. The Phase 1 guide recorded this temporary alias for Mahesha's workspace; it is not verified on Nicholas's Mac. Phase 1 recorded Nicholas's SDK at `/Users/nicholasbay/Documents/school/INF2004/pico-sdk`; verify your installation or follow [getting started](../GETTING_STARTED.md). The original teammate alias example is:

```sh
export PICO_SDK_PATH=/private/tmp/inf2004-maker-sdk
```

If that temporary alias is absent after a reboot, recreate it only when the target path is free:

```sh
ln -s "/Users/mahesha/Documents/INF2004 - Embedded Systems/INF2004/pico/pico-sdk" /private/tmp/inf2004-maker-sdk
```

On another laptop use its own full SDK through a path without spaces. Run these profiles sequentially because they share a build directory; profile switching discards incompatible objects.

| Board / purpose | Flags after `make -C build_make -j6` | Output under `build_make/` |
| --- | --- | --- |
| Maker Pi Pico with ordinary Pico, USB development | `SMP=1 CONSOLE=usb_cdc WIFI=none NET=none MQTT=0 CRASH_REPORT=0 TEST=1` | `mtk3pico_smp1_usb_cdc_test.uf2` |
| Pico W/WH, inhibited USB checks | `SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=none MQTT=0 CRASH_REPORT=0 TEST=1` | `mtk3pico_smp1_usb_cdc_wifi_test.uf2` |
| Pico W/WH on Robo Pico, future battery-only motor trial | `SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=none MQTT=0 CRASH_REPORT=0 TEST=0` | `mtk3pico_smp1_usb_cdc_wifi.uf2` |

Example for the Maker:

```sh
make -C build_make -j6 PICO_SDK_PATH=/private/tmp/inf2004-maker-sdk SMP=1 CONSOLE=usb_cdc WIFI=none NET=none MQTT=0 CRASH_REPORT=0 TEST=1
```

`WIFI=none` is the ordinary Pico profile; select the radio profile only if the installed module is actually W/WH. Both TEST profiles compile the real S3 path but S2 always calls brake, so an enabled request does not drive motors. The Maker Pi Pico is a development carrier, not the Robo motor driver. Leave its microSD slot empty (GP10/11 overlap motor outputs), and do not press its GP22 button while this Robo-oriented firmware drives GP22 for buzzer feedback. Do not assume its onboard feedback wiring matches the Robo carrier. Verify the installed board/module against the [Cytron Maker Pi Pico documentation](https://my.cytron.io/p-maker-pi-pico).

## Flash and use the template

1. Use USB as the sole power source for development. Hold BOOTSEL while connecting, then copy the matching **TEST=1** UF2 to RPI-RP2. Open the USB serial port after reboot and inspect startup/self-test output and `[BENCH] ... MOTOR_INHIBITED`. The periodic bench output reports ADC/calibration/state, not left/right duty values. See [Raspberry Pi BOOTSEL instructions](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html).
2. On the Maker, develop IR calculation and state logic using controlled software inputs first. Real ADC acquisition needs correctly connected sensors on GP26/27 with compatible electrical levels and common ground. Floating ADC readings or startup messages are not evidence that sensors work. Hardware calibration depends on mounted sensors and correct button/feedback wiring.
3. On the correctly wired Robo bench, begin stopped. Release then press GP20 with both sensors over black at mounted height; keep the pose for at least 500 ms. One chirp indicates black capture. Repeat over white; two chirps indicate valid white capture, three indicate rejection. Each channel requires >=300 ADC counts of contrast. Failed white contrast clears both references.
4. Place the calibrated sensors on a visible line, release and freshly press GP20 to start. Fully white or broad black rejects start. TEST firmware can exercise the software state transitions while keeping motors inhibited.
5. GP21 requests the independent persistent stop and cancels capture. Release GP21, then make a fresh GP20 press to reset STOPPED to READY. Release GP20 and press again to start. Holding a button cannot restart. Reset retains valid calibration; reboot loses it.
6. Physical motor trials remain pending. When a later approved trial is ready, flash the normal image, disconnect USB, then power from the agreed battery/power bank alone. Verify wiring, polarity and raised-wheel operation before floor trials. Follow the project's single-power-source policy and [Robo Pico hardware guidance](https://my.cytron.io/p-robo-pico). Building normal firmware here is not permission or evidence for a powered trial.

Recovery remains the existing proportional bench policy: both-white loss requests one wheel at 0.10 and the other at zero in the last known direction, for at most 100 ms. No known direction stops; two distinct fresh visible acquisitions can resume following. Broad black slows both wheels to 0.05 and cannot qualify for recovery. See the line catalogue for exact classification and steering rules.

## Verification and known limits

The original Phase 1 host checks covered tracking, paired ADC, robot policy and real S3/S2 task code separately in normal and TEST configurations. Later checks covered S1 encoding/snapshots and the MQTT adapter. Their runners are retired; saved [integration evidence](../../testing/integration/TEST_LOG.md) remains. Firmware self-tests are preserved; use [testing](../../testing/README.md) and [getting started](../GETTING_STARTED.md#build-profiles) for their current workflow. The [wireless extension](COMMUNICATIONS.md) reports requested duties, separately from the inhibited ordinary-Pico profile. Fake GPIO/ADC/S2/kernel/network checks did not run both real tasks together on the board. [System trials](../../testing/system/TEST_LOG.md) remain physical gates.

Current stop/reset has ordering limits: a stop can arrive between S2's flag check and its PWM write; reset has no acknowledged-brake barrier or generation protection against concurrent stop/request activity. `MotionHealth.stopped` is a latch, not an acknowledgement that wheels are braked/stationary. Stop latency and physical braking are unmeasured. Do not present host checks as proof that these races are fixed. Planned Phases 3 and 4 address them; Phase 2 adds hardware-boundary validation/error propagation.

Apply the adopted [project coding standard](../CODING_STANDARD.md), including the existing GNU C11 build and C11 atomics. The original handoff's mandatory-assignment attribution is not established by the supplied assignment text. No complete Barr compliance audit is claimed.

## Questions to settle before motor trials or interface changes

- Which Pico module is installed on each carrier: ordinary Pico or W/WH, and will the same module move between carriers?
- Are left/right sensor wiring, mounted height, black/white contrast and GPIO voltage levels verified on the actual bench?
- What is the present motor failure (power, wiring, polarity, driver configuration or software), and who will record the raised-wheel baseline? This phase does not diagnose it.
- What stop/reset ordering guarantee should the team require before physical acceptance? The existing latch is not a brake acknowledgement.
- How will battery-only results be observed/recorded under the single-power-source policy, and what steering/stop acceptance thresholds will the team use?

Phase 1 covers this development handoff. Encoder/PID additions, IR algorithm replacement and motor-runtime hardening are outside its scope. Course/calibration decisions are tracked in [PROJECT_SPECS](../PROJECT_SPECS.md#pending-decisions).

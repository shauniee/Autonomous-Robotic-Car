# Getting started: CSC01 micro T-Kernel IR bench

The [Word report](<../../CSC01 Design Review.docx>) is the integrated revised proposal. Start with [PROJECT.md](../PROJECT.md), [TEAM.md](../TEAM.md), [hardware](HARDWARE.md), [architecture](ARCHITECTURE.md) and the subsystem guides. Template README files remain intact; the former SDK/MQTT documentation is preserved under [history](history/pre-mtkernel/GETTINGS_STARTED.md) as historical material.

## 1. Build and software checks

From `code/`, set `PICO_SDK_PATH` to your Pico SDK checkout, with TinyUSB and CYW43 submodules installed, and put `arm-none-eabi-gcc` on PATH:

```sh
sh scripts/check-ir-tracking.sh
make -C build_make -j6 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=none MQTT=0 CRASH_REPORT=0 TEST=1
make -C build_make -j6 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=none MQTT=0 CRASH_REPORT=0 TEST=0
```

Use `mtk3pico_smp1_usb_cdc_wifi_test.uf2` for USB checks: Motion always brakes in this profile. Normal battery trial firmware is `mtk3pico_smp1_usb_cdc_wifi.uf2`. Profile switching automatically discards incompatible objects. Do not flash the retired CMake `build-ir-tracking/ir_tracking.uf2`.

## 2. Flash, calibrate and run

1. Use **USB alone**. Hold BOOTSEL during connection and copy the chosen UF2 to RPI-RP2. USB checks use TEST=1 with inhibited outputs. Verify the startup/test console and disconnected/raised wheels as appropriate. No on-target test result is currently recorded.
2. For a powered trial, flash the normal image, disconnect USB, then connect the power bank as the sole source. Never reconnect USB while that source is connected. A reboot loses calibration.
3. With the motors stopped, place both IR sensors fully over black at mounted height. Release then press GP20; hold the pose for at least 500 ms. One chirp means black captured.
4. Place both over white. Release then press GP20; hold for at least 500 ms. Two chirps mean valid calibration. Three mean rejection; inspect contrast/pose and repeat. Each channel needs >=300 counts contrast. Failed white contrast clears both references and returns to black.
5. Place on a visible line. Release then press GP20 to start. Fully white and broad black reject start. Check motor polarity with wheels raised before a low-speed floor run. GP21 stops immediately through the independent latch/wake path; it also cancels capture.
6. After stopping, release GP21 and use a fresh GP20 press to reset to Ready. Release GP20, then press again to start. Reset preserves valid calibration; held inputs cannot restart.

## 3. Proportional following and recovery

S3 now uses proportional control only (Kp=0.10), a later Nicholas-requested bench override from the unchanged Word report's PID version. Negative error requests left correction; positive requests right. The [line guide](architecture/LINE_FOLLOWING.md) contains all nine observation combinations, software states and equations. [IR_FUTURE_PLANS.md](architecture/IR_FUTURE_PLANS.md) records future PD/PID and barcode work.

During valid both-white loss, S3 requests one wheel at duty 0.10 and the other at zero in the last known direction, for at most 100 ms. No known direction means immediate stop. Two consecutive distinct fresh visible samples, each with a fraction >=0.25, return to P following; broad black cannot qualify. Invalid/stale sensing or GP21 stops immediately. Timeout requires a local reset and a separate fresh start, after repositioning. These are software policies, not measured physical recovery guarantees.

GP22 buzzer feedback is asynchronous; check the carrier mute switch. Initial base duty is 0.10, cap 0.35, lost-line recovery limit 100 ms. Motor polarity, stop latency, ADC ranges and reliable tracking remain **physical tests pending**.

## 4. Milestones and evidence

- Wiring/electrical release: measure rails, signal highs and currents; confirm actual modules, motor polarity, shared Grove GP26 and open GP28 jumper.
- Bench validation: USB test console, battery black/white capture, stop/reset, straight/curve tracking, line-loss/reboot. Record in `testing/`; never label a build as a hardware pass.
- Full mission: encoder control/actions, barcode mapping, terrain/reference height, sonar/stationary profiling, bounded bypass/original-line recovery and mission telemetry remain pending. Keep requirements from the report; settle course rules and numerical targets before trials.

## 5. Wireless telemetry on the Mac

USB alone is a supported Robo Pico supply: connect the Pico micro-B port to the computer and turn the carrier switch on, as described in [Cytron's power guide](https://my.cytron.io/tutorial/get-started-robo-pico-circuitpython-power-up-robo-pico). Disconnect the power bank first. Use TEST=1 for USB sensor/console checks; its motor outputs are inhibited. Driving uses the power bank alone after unplugging USB.

Mosquitto is already installed on this Mac. On another Mac, install it with `brew install mosquitto`. Use a reachable private 2.4 GHz WiFi network shared by Pico and Mac. Guest/client isolation, a VPN or a firewall can prevent the Pico reaching the broker. This bench configuration allows anonymous unencrypted access on TCP 1883 and is intended only for that private network.

From `code/`, create `config/wifi_secrets.h` from `config/wifi_secrets.example.h` **only if it is missing**, then edit the local file:

```sh
test -f config/wifi_secrets.h || cp config/wifi_secrets.example.h config/wifi_secrets.h
ipconfig getifaddr en0
```

Set `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_BROKER_IP` to the Mac's reachable LAN IPv4 address, port 1883, client ID `csc01-s3-pico` and topic `robot/s3/telemetry`. `en0` must be the active WiFi interface; use the WiFi settings' TCP/IP address if it is different. Do not use 127.0.0.1 for the Pico broker address. The local file is ignored; do not commit credentials or firmware binaries. Rebuild after changing it; move networks only after updating the broker address.

In terminal 1, from `code/`:

```sh
mosquitto -c config/mosquitto-lab.conf
```

In terminal 2, also from `code/`:

```sh
mosquitto_sub -h 127.0.0.1 -p 1883 \
  -t robot/s3/telemetry -v
```

`mosquitto_sub` prints incoming messages directly; `-v` includes the topic. See the [official subscriber documentation](https://mosquitto.org/man/mosquitto_sub-1.html). Leave both terminals running. If port 1883 is already occupied by your broker, reuse it instead of launching a second instance. Allow the selected broker's incoming connections in the Mac firewall if prompted.

With `PICO_SDK_PATH` and the ARM compiler set as in section 1, build these profiles sequentially:

```sh
make -C build_make -j6 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 TEST=1
make -C build_make -j6 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 TEST=0
make -C build_make -j6 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=none MQTT=0 CRASH_REPORT=0 TEST=0
```

| Profile | File inside build_make/ | Use |
| --- | --- | --- |
| Wireless TEST | mtk3pico_smp1_usb_cdc_wifi_lwip_mqtt_test.uf2 | USB-alone connectivity, calibration and self-checks; motor outputs always inhibited |
| Wireless normal | mtk3pico_smp1_usb_cdc_wifi_lwip_mqtt.uf2 | Power-bank-alone driving with wireless observations |
| Offline normal | mtk3pico_smp1_usb_cdc_wifi.uf2 | Existing local bench without MQTT |

First flash wireless TEST over USB alone and verify messages roughly every 500 ms, `test_inhibited:true`, changing raw values and completed black/white references. GP20 still captures black, captures white and starts on separate released presses. `motion_enabled:true` reports the controller's request; it does not override TEST motor inhibition. Then flash wireless normal, unplug USB, connect only the power bank, recalibrate and use the same subscriber while running. Reboot clears calibration and resets the stream sequence.

The [payload guide](architecture/COMMUNICATIONS.md) explains `null` values, acquisition age and failure behavior. A 500 ms stream can miss the entire 100 ms recovery turn: use an instrument/camera for physical timing. Actual Pico connectivity and driving results must be recorded in [system tests](../testing/system/TEST_LOG.md); a successful build or a host fixture message is not a Pico pass.
- Team review: confirm the revised shared files and priority proposal. No teammate/instructor approval is asserted by this migration.
- Week 6 report has no five-page limit. Host/build evidence is separate from proposed T01-T16 hardware trials and later Week 8/10 assessment work.

Logs require exact source identification. The saved 2026-10-08 software checks ran before the migration was committed, so their evidence records a source SHA256 manifest and base Git revision rather than mislabelling HEAD as the tested firmware revision. Preserve those records; use the resulting committed revision and flashed-image checksum for formal on-target trials.

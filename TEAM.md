# CSC01 team, ownership and interfaces

Migration requested by Nicholas on 8 October 2026. Shared configuration and priority allocation are a **revised proposal for team review**, not recorded teammate/instructor approval. The proportional-only S3 change and read-only MQTT addition are later Nicholas-requested bench overrides; the Word report is preserved. The MQTT adapter and its output-ring setting are intentional shared-library changes under that request. Original template guidance, including owner-edit and shared-file agreement rules, is preserved in [UPSTREAM_TEAM.md](docs/UPSTREAM_TEAM.md).

## 1. Members and subsystems

| # | Owner / student ID | Responsibility | Active status |
|---|---|---|---|
| S1 | Shaun / 2500629 | Communications | Latest S3 telemetry snapshot, mutex and bounded JSON; kernel service publishes; no S1 task |
| S2 | Mahesha / 2503973 | Motion / motors / encoders | Motor PWM and local expiry implemented; encoders pending |
| S3 | Nicholas / 2501552 | Line/barcode; integration | Proportional IR, button calibration, explicit bounded recovery and watchdog; barcode pending |
| S4 | Darrius / 2500360 | Terrain | Inactive placeholder |
| S5 | Jia Jing / 2502825 | Obstacle | Inactive placeholder |

Folders are `app_program/subsystemN/`, with corresponding `testing/subsystemN/`. Unknown GitHub usernames are left unspecified.

## 2. Pins

| Pins | Owner | Current / proposed use |
|---|---|---|
| GP23/24/25/29 | Kernel | Pico W radio; never reassign |
| GP0/1, GP7/28 | S2 | Future encoders; CRASH_REPORT=0; GP28 battery jumper open |
| GP8/9, GP10/11 | S2 | M1 left, M2 right; LOW/LOW brake; physical polarity pending |
| GP26/27 | S3 | Left ADC0 / right ADC1; Grove 6 shared GP26 contact disconnected |
| GP20/21 | S3 | Active-low calibrate/start/reset and stop buttons |
| GP22 | S3 | Buzzer feedback; onboard mute switch must allow sound |
| GP16 | S3 | Future barcode capture; inactive |
| GP4/5 | S4 | Future GY-511 I2C0; inactive |
| GP2/3, GP12 | S5 | Future sonar and servo; inactive |
| GP6/13/14/15/17/18/19 | Unclaimed | No demonstration LED or other example output |

## 3. Peripherals

| Peripheral | Owning task | Configuration |
|---|---|---|
| ADC | S3 IR bench | Native device ADC disabled (`DEVCNF_USE_ADC=0`); bounded `drv_ir.c` |
| PWM4/5 | S2 Motion | GP8-11, 20 kHz at template 125 MHz clock |
| PWM3 | S3 IR bench | GP22, asynchronous 2 kHz chirps |
| PWM block reset release | S2 init | Release once without resetting; individual slices have separate owners; USE_PTMR=0 |
| Watchdog tick / hardware TIMER | S3 init | Enable 1 us tick from 12 MHz reference; release TIMER without resetting existing time. Kernel schedule remains SysTick; WiFi reads TIMER |
| Watchdog | S3 IR bench | 250 ms; feed only on fresh, advancing IR AND Motion progress |
| USB / CYW43439 | Kernel | Service tasks priorities 3/4, core 0 |
| I2C0 / sonar / encoders | Future subsystem owners | No active drivers in this bench |

### WiFi and MQTT are special

The radio and the network stack belong to the kernel's built-in WiFi service
task, `cyw43_utk_task` (`lib/libwifi/sysdepend/pico_rp2040/`). **No student
task may call `cyw43_...`, lwIP or MQTT functions, not even from a
"WiFi subsystem".** The calls are not safe from any other task, on either core.

What students *can* do:

- **Read the connection state** from any task, which is always safe:
  ```c
  #include "cyw43_utk.h"
  T_CYW43_UTK_STATUS st;
  cyw43_utk_get_status(&st);   /* st.ready, st.link_status, st.ip_addr, ... */
  ```
- **Publish telemetry inside the service task.** The requested adapter in
  `lib/liblwip/sysdepend/pico_rp2040/mqtt_utk.c` replaces the template demo
  heartbeat/echo subscription. S3 fills S1's portable snapshot through IF-05;
  the service copies it with `TMO_POL`, formats JSON and publishes every
  500 ms. There are no subscriptions or remote controls. It needs
  `NET=lwip MQTT=1`; the offline profile still works.

The upstream shared-file agreement rule remains a team process requirement;
this requested bench extension does not record teammate/instructor approval.
Networking remains exclusive to the existing core-0 service.

## 4. Task priorities

1 is highest; kernel priorities 1-4 remain reserved. This bench deliberately departs from the template example bands. Acceptance by the team remains pending.

| Task | Priority | Core | Period / event |
|---|---:|---:|---|
| S3 ir_task (buttons, ADC, calibration, P steering, recovery, feedback, health) | 12 | 1 (TP_PRC2) | 10 ms buttons, 20 ms paired ADC |
| S2 Motion | 13 | 1 (TP_PRC2) | 20 ms or immediate stop wake-up |

No application networking task exists. Future full mission split: Safety 12, Motion 13, Mission 14, Line/Barcode 15, Terrain 16, Obstacle 17, S1 bridge 20; all application core 1, kernel services core 0. This split is proposed, not implemented.

## 5. Interfaces and kernel objects

| ID | Producer -> consumer | Owner / object | Contract |
|---|---|---|---|
| IF-01 | S3 -> S2 | S2 priority-inheritance mutex + latest MotionRequest | Bounded duties 0..0.35, acquisition ms, enabled. Expired >60 ms or invalid active request brakes and latches stop |
| IF-02 | S3 -> S2 | S2 persistent event flag + tk_wup_tsk | Stop independent of request mutex. Separate local reset clears flag and invalidates old request; reset alone cannot move |
| IF-03 | S2 -> S3 | S2 mutex-protected MotionHealth | Completed output-service timestamp and sequence; no encoder speed claimed |
| IF-04 | IR calculations -> S3 coordinator | In-task IrTrackingOutput | Fresh visible-steering validity, condition, signed error, memoryless P steering [-1,1], original acquisition time |
| IF-05 | S3 -> S1 snapshot -> kernel network service | S1 priority-inheritance mutex + latest S1Telemetry | Portable fixed-size value copy; producer and consumer use TMO_POL. Busy/unavailable object skips diagnostics. Sequence advances only on stored updates; original sample time is preserved. JSON <=768 bytes; QoS 0, no retain, 500 ms; no commands |

Use mutex copies for latest snapshots; micro T-Kernel mailboxes pass pointers and require lifetime management, not FreeRTOS length-one overwrite semantics. Future navigation FIFO should use a bounded message buffer or explicit pooled mailbox messages; no navigation FIFO is implemented here.

## 6. Kernel object budget for the active S3 bench

| Object | Configured application limit | S3 allocated | S3 use |
|---|---:|---:|---|
| Task | 32 | 1 | ir_task, priority12/core1, existing 2048-byte stack |
| Semaphore | 16 | 0 | none |
| Event flag | 16 | 0 | S3 uses S2's persistent stop flag via IF-02 |
| Mailbox | 12 | 0 | none |
| Mutex | 12 | 0 | S3 uses S2's request/health mutex via IF-01/03 |
| Message buffer | 12 | 0 | future barcode FIFO is not allocated |
| Fixed memory pool | 12 | 0 | none |

This is S3 usage, not the total including S2 and kernel services. Existing configuration limits are unchanged; stack margin/capacity under future load require measurement. S3 private raw samples/capture sums/control state are task-local and use no additional kernel object. The one IrCaptureKind and typed IrCaptureResult replace legacy command IDs/string-based branching.

For MQTT=1, S1 adds **one mutex, zero tasks**. `subsystem1_init()` creates it; `subsystem1_start()` does not start a task. The kernel service may run before application initialization, so an atomic object ID gates access. Allocation failure disables diagnostics while local startup continues. Snapshot copying neither waits for a mutex nor feeds the watchdog. For MQTT=0 no S1 mutex is allocated and S3 omits the handoff.

## 7. Startup and power

Create all subsystem objects before starting any application task. Required initialization failure prevents startup; a task-start failure returns its error. Optional telemetry allocation failure skips diagnostics. Motors are inhibited at boot. TEST=1 also inhibits all motor outputs in the Motion task.

**One power source at a time.** USB alone for flashing and inhibited desktop checks. Disconnect USB before power-bank operation; never connect it to collect logs during a powered trial. References live in RAM: calibrate again after the power change.

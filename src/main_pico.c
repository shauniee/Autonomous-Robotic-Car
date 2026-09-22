#include "pico/stdlib.h"
#include "robot/config.h"
#include "robot/controller.h"
#include "robot/subsystems.h"

int main(void) {
    /* Runs once after boot. USB stdio lets printf() send text to a serial monitor. */
    stdio_init_all();
    /* Prepare each buddy's module before entering the repeating control loop. */
    motion_init();
    line_barcode_init();
    terrain_init();
    obstacle_init();
    comms_init();
    /* The controller stores the mission state and requested left/right speeds. */
    VehicleController controller;
    controller_init(&controller);
    uint32_t last_telemetry_ms = 0;

    while (true) {
        /* Uptime in milliseconds, not the calendar time on your laptop. */
        const uint32_t now_ms = to_ms_since_boot(get_absolute_time());
        SensorFrame sensors = {
            /* Collect one snapshot of the current readings from buddies 3-5. */
            .line = line_barcode_update(now_ms),
            .terrain = terrain_update(now_ms),
            .obstacle = obstacle_update(now_ms)
        };
        RobotCommand command = comms_poll(now_ms);
        /* comms_poll services WiFi/MQTT and returns at most one queued command.
           It returns COMMAND_NONE when there is nothing new to act on. */
        /* USB bench commands: s=start, x=stop. Stop takes priority. */
        const int key = getchar_timeout_us(0);
        /* Timeout 0 means check the keyboard/serial input without waiting. */
        if (key == 'x') command = COMMAND_STOP;
        else if (key == 's' && command != COMMAND_STOP) command = COMMAND_START;

        /* '&' passes an address so these functions can access the existing data.
           The controller decides whether a received START is currently allowed. */
        controller_step(&controller, &sensors, command, now_ms);
        if (controller.state == STATE_FOLLOW_LINE) {
            motion_update(controller.target, now_ms);
        } else {
            motion_stop();
        }
        const MotionFeedback feedback = motion_read();
        /* Telemetry reports actual encoder feedback separately from target speeds.
           Publish periodically, rather than sending a packet on every loop. */
        if ((uint32_t)(now_ms - last_telemetry_ms) >= TELEMETRY_PERIOD_MS) {
            comms_publish(&controller, &sensors, &feedback, now_ms);
            last_telemetry_ms = now_ms;
        }
        /* Starter loop: effective period includes work above. Replace with
           measured deadline scheduling before tuning a fixed-period PID. */
        sleep_ms(CONTROL_PERIOD_MS);
    }
}

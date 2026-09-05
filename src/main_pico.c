#include "pico/stdlib.h"
#include "robot/config.h"
#include "robot/controller.h"
#include "robot/subsystems.h"

int main(void) {
    stdio_init_all();
    motion_init();
    line_barcode_init();
    terrain_init();
    obstacle_init();
    comms_init();
    VehicleController controller;
    controller_init(&controller);
    uint32_t last_telemetry_ms = 0;

    while (true) {
        const uint32_t now_ms = to_ms_since_boot(get_absolute_time());
        SensorFrame sensors = {
            .line = line_barcode_update(now_ms),
            .terrain = terrain_update(now_ms),
            .obstacle = obstacle_update(now_ms)
        };
        RobotCommand command = comms_poll(now_ms);
        /* USB bench commands: s=start, x=stop. Stop takes priority. */
        const int key = getchar_timeout_us(0);
        if (key == 'x') command = COMMAND_STOP;
        else if (key == 's' && command != COMMAND_STOP) command = COMMAND_START;

        controller_step(&controller, &sensors, command, now_ms);
        if (controller.state == STATE_FOLLOW_LINE) {
            motion_update(controller.target, now_ms);
        } else {
            motion_stop();
        }
        const MotionFeedback feedback = motion_read();
        if ((uint32_t)(now_ms - last_telemetry_ms) >= TELEMETRY_PERIOD_MS) {
            comms_publish(&controller, &sensors, &feedback, now_ms);
            last_telemetry_ms = now_ms;
        }
        /* Starter loop: effective period includes work above. Replace with
           measured deadline scheduling before tuning a fixed-period PID. */
        sleep_ms(CONTROL_PERIOD_MS);
    }
}

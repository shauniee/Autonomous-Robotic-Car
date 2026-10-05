#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "robot/sensor_bench.h"
#include "robot/subsystems.h"
#include "robot/comms_protocol.h"
#include <stdio.h>
/* Sensors publish from boot. No autonomous driving or motor PWM is linked. */
int main(void) {
    SensorBench sensors;
    sensor_bench_init(&sensors);
    stdio_init_all();
    gpio_init(20); gpio_set_dir(20, GPIO_IN); gpio_pull_up(20);
    gpio_init(21); gpio_set_dir(21, GPIO_IN); gpio_pull_up(21);
    comms_init();
    watchdog_enable(1000, true);
    uint32_t last_telemetry = 0, start_changed = 0;
    bool candidate = false, stable = false, armed = false;
    static char payload[COMMS_TELEMETRY_MAX];
    puts("sensor_bench_mqtt: motors disabled; b/w=IR calibration, a=scan, c=center, x=servo off. GP20=scan toggle, GP21=servo off.");
    while (true) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        bool down = !gpio_get(20), stop = !gpio_get(21);
        int key = getchar_timeout_us(0);
        if (down != candidate) { candidate = down; start_changed = now; }
        if ((uint32_t)(now - start_changed) >= 30u) {
            if (!candidate) armed = true;
            if (stable != candidate) {
                stable = candidate;
                if (stable && armed) {
                    armed = false;
                    if (key != 'x') key = sensors.scan_enabled ? 'c' : (sensors.servo_enabled ? 'a' : 'c');
                }
            }
        }
        if (stop) key = 'x'; /* Held stop always wins. */
        if (key >= 0 && !(stop && !sensors.servo_enabled)) sensor_bench_command(&sensors, key, now);
        sensor_bench_update(&sensors, now);
        (void)comms_poll(now);
        now = to_ms_since_boot(get_absolute_time());
        if (comms_status_requested() || (uint32_t)(now - last_telemetry) >= 500u) {
            if (sensor_bench_encode(payload, sizeof(payload), &sensors, now)) comms_publish_json(payload);
            last_telemetry = now;
        }
        watchdog_update(); sleep_ms(1);
    }
}

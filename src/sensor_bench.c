#include "robot/sensor_bench.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/sync.h"
#include <string.h>

#define I2C_TIMEOUT_US 2000u
static uint32_t last_adc_ms, last_imu_ms, last_config_ms;
static bool have_black, have_white;
static volatile bool capture_active, echo_high, echo_complete;
static volatile uint32_t rise_us, width_us, fall_us;
static uint32_t trigger_us, settle_until_ms;
static bool waiting_echo, gap_wait;
static unsigned sweep_index;
static const unsigned sweep[] = {90, 60, 90, 120};

static bool read_reg(uint8_t addr, uint8_t reg, uint8_t *out, size_t count) {
    return i2c_write_timeout_us(i2c0, addr, &reg, 1, true, I2C_TIMEOUT_US) == 1 &&
           i2c_read_timeout_us(i2c0, addr, out, count, false, I2C_TIMEOUT_US) == (int)count;
}
static bool write_reg(uint8_t addr, uint8_t reg, uint8_t value) {
    uint8_t bytes[] = {reg, value};
    return i2c_write_timeout_us(i2c0, addr, bytes, 2, false, I2C_TIMEOUT_US) == 2;
}
static void configure_imu(SensorBench *s, uint32_t now) {
    if (!s->accel_configured) {
        uint8_t ctrl;
        s->accel_configured = write_reg(BENCH_ACCEL_ADDR, 0x20, 0x47) && /* 50Hz, XYZ */
            write_reg(BENCH_ACCEL_ADDR, 0x23, 0x88) && /* BDU, HR, +/-2g */
            read_reg(BENCH_ACCEL_ADDR, 0x23, &ctrl, 1) && ctrl == 0x88;
        if (!s->accel_configured) ++s->i2c_errors;
    }
    if (!s->mag_configured) {
        uint8_t id[3];
        s->mag_configured = read_reg(BENCH_MAG_ADDR, 0x0a, id, 3) &&
            id[0] == 0x48 && id[1] == 0x34 && id[2] == 0x33 &&
            write_reg(BENCH_MAG_ADDR, 0x00, 0x14) && /* 30Hz */
            write_reg(BENCH_MAG_ADDR, 0x01, 0x20) && /* +/-1.3 gauss */
            write_reg(BENCH_MAG_ADDR, 0x02, 0x00); /* continuous */
        if (!s->mag_configured) ++s->i2c_errors;
    }
    last_config_ms = now;
}
static void imu_update(SensorBench *s, uint32_t now) {
    if ((uint32_t)(now - last_config_ms) >= 2000u) configure_imu(s, now);
    if ((uint32_t)(now - last_imu_ms) < 20u) return;
    last_imu_ms = now;
    uint8_t status, bytes[6];
    if (s->accel_configured) {
        if (!read_reg(BENCH_ACCEL_ADDR, 0x27, &status, 1)) {
            s->accel_configured = false; s->accel.status.valid = false; ++s->i2c_errors;
        } else if (status & 0x08) {
            if (read_reg(BENCH_ACCEL_ADDR, 0x28 | 0x80, bytes, 6)) {
                bench_decode_accel(bytes, &s->accel);
                s->accel.status = (SampleStatus){true, now};
            } else { s->accel.status.valid = false; s->accel_configured = false; ++s->i2c_errors; }
        }
    }
    if (s->mag_configured) {
        if (!read_reg(BENCH_MAG_ADDR, 0x09, &status, 1)) {
            s->mag_configured = false; s->magnetic.status.valid = false; ++s->i2c_errors;
        } else if (status & 0x03) { /* Read a locked full frame too, to release LOCK. */
            if (read_reg(BENCH_MAG_ADDR, 0x03, bytes, 6)) {
                s->magnetic.status = (SampleStatus){bench_decode_magnetic(bytes, &s->magnetic), now};
            } else { s->magnetic.status.valid = false; s->mag_configured = false; ++s->i2c_errors; }
        }
    }
    s->attitude_valid = s->accel.status.valid && (uint32_t)(now - s->accel.status.sampled_ms) <= BENCH_SAMPLE_MAX_AGE_MS &&
        bench_tilt(s->accel.x, s->accel.y, s->accel.z, &s->pitch_deg, &s->roll_deg);
}
static void echo_irq(unsigned gpio, uint32_t events) {
    if (gpio != BENCH_ECHO_GPIO || !capture_active) return;
    const uint32_t now = time_us_32();
    if ((events & GPIO_IRQ_EDGE_RISE) && !echo_high) { rise_us = now; echo_high = true; }
    if ((events & GPIO_IRQ_EDGE_FALL) && echo_high) {
        width_us = now - rise_us; fall_us = now;
        echo_complete = true; capture_active = false;
    }
}
static void servo_set(SensorBench *s, unsigned angle, uint32_t now) {
    s->servo_command_deg = angle; s->servo_pulse_us = bench_servo_pulse(angle);
    s->servo_command_ms = now; s->servo_enabled = true;
    pwm_set_gpio_level(BENCH_SERVO_GPIO, s->servo_pulse_us);
    settle_until_ms = now + BENCH_SERVO_SETTLE_MS;
}
static void store_range(SensorBench *s, uint32_t now, bool valid, uint32_t pulse) {
    BenchRange result = {.status={valid, now}, .angle_deg=s->servo_command_deg, .echo_us=pulse};
    if (valid) result.status.valid = bench_echo_distance(pulse, &result.distance_m);
    s->range = result;
    if (result.angle_deg == 90) s->forward = result;
    for (unsigned i = 0; i < BENCH_SCAN_COUNT; ++i) {
        if (result.angle_deg == 60u + 30u * i) s->scan[i] = result;
    }
    if (!valid) ++s->sonar_timeouts;
}
static void sonar_update(SensorBench *s, uint32_t now) {
    if (!s->servo_enabled) return;
    if (waiting_echo) {
        uint32_t lock = save_and_disable_interrupts();
        bool done = echo_complete;
        uint32_t pulse = width_us, ended = fall_us;
        restore_interrupts(lock);
        if (done || (uint32_t)(time_us_32() - trigger_us) >= BENCH_ECHO_TIMEOUT_US) {
            lock = save_and_disable_interrupts(); capture_active = false; echo_complete = false; restore_interrupts(lock);
            bool valid = done && (uint32_t)(ended - trigger_us) <= BENCH_ECHO_TIMEOUT_US;
            /* Acquisition time is the captured edge, never the later network poll. */
            uint32_t acquired = valid ? now - (uint32_t)(time_us_32() - ended) / 1000u : now;
            store_range(s, acquired, valid, done ? pulse : 0);
            waiting_echo = false; gap_wait = true;
        }
        return;
    }
    if (gap_wait) {
        if ((uint32_t)(time_us_32() - trigger_us) < BENCH_TRIGGER_GAP_US) return;
        gap_wait = false;
        if (s->scan_enabled) {
            sweep_index = (sweep_index + 1u) % 4u;
            if (sweep[sweep_index] != s->servo_command_deg) servo_set(s, sweep[sweep_index], now);
        } else if (s->servo_command_deg != 90) servo_set(s, 90, now);
    }
    if ((int32_t)(now - settle_until_ms) < 0) return;
    if (gpio_get(BENCH_ECHO_GPIO)) { /* Stuck-high or old pulse is not clear space. */
        trigger_us = time_us_32(); store_range(s, now, false, 0); gap_wait = true; return;
    }
    uint32_t lock = save_and_disable_interrupts();
    echo_complete = false; echo_high = false; capture_active = true;
    trigger_us = time_us_32();
    restore_interrupts(lock);
    gpio_put(BENCH_TRIG_GPIO, 1); busy_wait_us_32(10); gpio_put(BENCH_TRIG_GPIO, 0);
    ++s->sonar_attempts; waiting_echo = true;
}
void sensor_bench_init(SensorBench *s) {
    memset(s, 0, sizeof(*s));
    /* No motor PWM configuration anywhere in this target. LOW/LOW = brake. */
    for (unsigned pin = 8; pin <= 11; ++pin) { gpio_init(pin); gpio_put(pin, 0); gpio_set_dir(pin, GPIO_OUT); }
    adc_init(); adc_gpio_init(BENCH_IR_GPIO); adc_select_input(0);
    i2c_init(i2c0, 100000); gpio_set_function(BENCH_SDA_GPIO, GPIO_FUNC_I2C); gpio_set_function(BENCH_SCL_GPIO, GPIO_FUNC_I2C);
    gpio_pull_up(BENCH_SDA_GPIO); gpio_pull_up(BENCH_SCL_GPIO);
    gpio_init(BENCH_TRIG_GPIO); gpio_put(BENCH_TRIG_GPIO, 0); gpio_set_dir(BENCH_TRIG_GPIO, GPIO_OUT);
    gpio_init(BENCH_ECHO_GPIO); gpio_set_dir(BENCH_ECHO_GPIO, GPIO_IN); gpio_pull_down(BENCH_ECHO_GPIO);
    gpio_set_irq_enabled_with_callback(BENCH_ECHO_GPIO, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, echo_irq);
    unsigned slice = pwm_gpio_to_slice_num(BENCH_SERVO_GPIO);
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, (float)clock_get_hz(clk_sys) / 1000000.0f); /* 1us ticks */
    pwm_config_set_wrap(&cfg, 19999); /* 50Hz */
    pwm_init(slice, &cfg, false); pwm_set_gpio_level(BENCH_SERVO_GPIO, 1500);
    gpio_set_function(BENCH_SERVO_GPIO, GPIO_FUNC_PWM); pwm_set_enabled(slice, true);
    uint32_t now = to_ms_since_boot(get_absolute_time());
    for (unsigned i = 0; i < BENCH_SCAN_COUNT; ++i) s->scan[i].angle_deg = 60u + 30u * i;
    servo_set(s, 90, now); configure_imu(s, now);
}
void sensor_bench_update(SensorBench *s, uint32_t now_ms) {
    if ((uint32_t)(now_ms - last_adc_ms) >= 20u) {
        uint32_t sum = 0; for (unsigned i = 0; i < 8; ++i) sum += adc_read();
        s->ir_raw = sum / 8u; s->ir_status = (SampleStatus){true, now_ms}; last_adc_ms = now_ms;
        s->ir_calibrated = have_black && have_white && bench_ir_fraction(s->ir_raw, s->ir_black, s->ir_white, &s->ir_black_fraction);
    }
    imu_update(s, now_ms);
    /* I2C may have consumed milliseconds; use actual time for servo deadlines. */
    sonar_update(s, to_ms_since_boot(get_absolute_time()));
}
void sensor_bench_command(SensorBench *s, int key, uint32_t now_ms) {
    if (key == 'b' && s->ir_status.valid) { s->ir_black = s->ir_raw; have_black = true; }
    if (key == 'w' && s->ir_status.valid) { s->ir_white = s->ir_raw; have_white = true; }
    if (key != 'a' && key != 'c' && key != 'x') return;
    uint32_t lock = save_and_disable_interrupts(); capture_active = false; echo_complete = false; restore_interrupts(lock);
    waiting_echo = false; gap_wait = false; sweep_index = 0;
    s->range.status.valid = false; s->forward.status.valid = false;
    s->scan_enabled = key == 'a';
    if (key == 'x') {
        pwm_set_gpio_level(BENCH_SERVO_GPIO, 0); s->servo_enabled = false;
    } else servo_set(s, 90, now_ms);
}

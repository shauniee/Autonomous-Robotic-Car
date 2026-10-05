#include "robot/sensor_bench.h"
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
typedef struct { char *out; size_t capacity, used; bool failed; } Writer;
static void add(Writer *w, const char *format, ...) {
    if (w->failed) return;
    va_list args; va_start(args, format);
    int n = vsnprintf(w->out + w->used, w->capacity - w->used, format, args);
    va_end(args);
    if (n < 0 || (size_t)n >= w->capacity - w->used) w->failed = true;
    else w->used += n;
}
static bool fresh(SampleStatus s, uint32_t now, uint32_t age) {
    return s.valid && (uint32_t)(now - s.sampled_ms) <= age;
}
static void num(Writer *w, const char *key, float value, bool valid) {
    add(w, ",\"%s\":", key);
    if (valid && isfinite(value)) add(w, "%.6g", (double)value); else add(w, "null");
}
static void status(Writer *w, SampleStatus s, uint32_t now, bool valid) {
    add(w, "\"valid\":%s,\"sampled_ms\":%" PRIu32 ",\"age_ms\":%" PRIu32,
        valid ? "true" : "false", s.sampled_ms, (uint32_t)(now - s.sampled_ms));
}
static void vector(Writer *w, const BenchVector *v, uint32_t now, const char *unit) {
    bool valid = fresh(v->status, now, BENCH_SAMPLE_MAX_AGE_MS);
    add(w, "{"); status(w, v->status, now, valid);
    add(w, ",\"unit\":\"%s\",\"raw\":[%d,%d,%d]", unit, v->raw[0], v->raw[1], v->raw[2]);
    num(w, "x", v->x, valid); num(w, "y", v->y, valid); num(w, "z", v->z, valid); add(w, "}");
}
static void range(Writer *w, const BenchRange *r, uint32_t now) {
    bool valid = fresh(r->status, now, BENCH_RANGE_MAX_AGE_MS);
    add(w, "{"); status(w, r->status, now, valid);
    add(w, ",\"commanded_angle_deg\":%u,\"echo_us\":%" PRIu32, r->angle_deg, r->echo_us);
    num(w, "distance_m", r->distance_m, valid); add(w, "}");
}
size_t sensor_bench_encode(char *out, size_t capacity, const SensorBench *s, uint32_t now) {
    if (!capacity) return 0;
    Writer w = {out, capacity, 0, false};
    add(&w, "{\"schema\":2,\"mode\":\"sensor_bench\",\"state\":\"MONITORING\",\"uptime_ms\":%" PRIu32
        ",\"motors\":{\"connected\":false,\"enabled\":false,\"left_mps\":null,\"right_mps\":null}", now);
    bool ir_valid = fresh(s->ir_status, now, BENCH_SAMPLE_MAX_AGE_MS);
    add(&w, ",\"ir_right\":{"); status(&w, s->ir_status, now, ir_valid);
    add(&w, ",\"gpio\":26,\"adc_channel\":0,\"raw_adc\":%u,\"calibrated\":%s,\"black_reference\":%u,\"white_reference\":%u",
        s->ir_raw, s->ir_calibrated ? "true" : "false", s->ir_black, s->ir_white);
    num(&w, "nominal_voltage_v", s->ir_raw * (3.3f / 4095.0f), ir_valid);
    num(&w, "black_fraction", s->ir_black_fraction, ir_valid && s->ir_calibrated); add(&w, "}");
    add(&w, ",\"imu\":{\"device\":\"LSM303DLHC\",\"accel_configured\":%s,\"mag_configured\":%s,\"i2c_errors\":%" PRIu32 ",\"acceleration\":",
        s->accel_configured ? "true" : "false", s->mag_configured ? "true" : "false", s->i2c_errors);
    vector(&w, &s->accel, now, "g"); add(&w, ",\"magnetic_field\":"); vector(&w, &s->magnetic, now, "gauss");
    bool attitude = s->attitude_valid && fresh(s->accel.status, now, BENCH_SAMPLE_MAX_AGE_MS);
    add(&w, ",\"attitude_valid\":%s", attitude ? "true" : "false");
    num(&w, "pitch_deg", s->pitch_deg, attitude); num(&w, "roll_deg", s->roll_deg, attitude);
    add(&w, ",\"yaw_rate_dps\":null,\"hump_height_m\":null}");
    add(&w, ",\"servo\":{\"gpio\":12,\"enabled\":%s,\"scan_enabled\":%s,\"commanded_angle_deg\":%u,\"pulse_us\":%u,\"commanded_ms\":%" PRIu32 ",\"actual_angle_deg\":null}",
        s->servo_enabled ? "true" : "false", s->scan_enabled ? "true" : "false", s->servo_command_deg,
        s->servo_enabled ? s->servo_pulse_us : 0, s->servo_command_ms);
    add(&w, ",\"ultrasonic\":{\"echo_gpio\":2,\"trig_gpio\":3,\"attempts\":%" PRIu32 ",\"timeouts\":%" PRIu32 ",\"latest\":", s->sonar_attempts, s->sonar_timeouts);
    range(&w, &s->range, now); add(&w, ",\"forward\":"); range(&w, &s->forward, now); add(&w, ",\"scan\":[");
    for (unsigned i = 0; i < BENCH_SCAN_COUNT; ++i) { if (i) add(&w, ","); range(&w, &s->scan[i], now); }
    add(&w, "]}}");
    if (w.failed) { out[0] = '\0'; return 0; }
    return w.used;
}

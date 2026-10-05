#include "robot/sensor_bench.h"
#include <math.h>
#include <stdlib.h>
static int16_t little(const uint8_t *p) { return (int16_t)((uint16_t)p[0] | (uint16_t)p[1] << 8); }
static int16_t big(const uint8_t *p) { return (int16_t)((uint16_t)p[1] | (uint16_t)p[0] << 8); }
void bench_decode_accel(const uint8_t bytes[6], BenchVector *v) {
    /* +/-2g, high-resolution mode, 12-bit left-justified, 1mg/LSB. */
    for (unsigned i = 0; i < 3; ++i) v->raw[i] = little(bytes + 2 * i) / 16;
    v->x = v->raw[0] * 0.001f; v->y = v->raw[1] * 0.001f; v->z = v->raw[2] * 0.001f;
}
bool bench_decode_magnetic(const uint8_t bytes[6], BenchVector *v) {
    /* Device order is X,Z,Y, big-endian; +/-1.3 gauss. */
    v->raw[0] = big(bytes); v->raw[1] = big(bytes + 4); v->raw[2] = big(bytes + 2);
    for (unsigned i = 0; i < 3; ++i) if (v->raw[i] == -4096) return false;
    v->x = v->raw[0] / 1100.0f; v->y = v->raw[1] / 1100.0f; v->z = v->raw[2] / 980.0f;
    return true;
}
bool bench_tilt(float x, float y, float z, float *pitch, float *roll) {
    float norm = sqrtf(x*x + y*y + z*z);
    if (!isfinite(norm) || norm < 0.8f || norm > 1.2f) return false;
    /* Sensor axes, gravity assumption; not chassis-calibrated attitude. */
    *pitch = atan2f(-x, sqrtf(y*y + z*z)) * 57.2957795f;
    *roll = atan2f(y, z) * 57.2957795f;
    return true;
}
bool bench_ir_fraction(uint16_t raw, uint16_t black, uint16_t white, float *fraction) {
    int span = (int)black - (int)white;
    if (raw > 4095 || black > 4095 || white > 4095 || abs(span) < 300) return false;
    float value = ((float)raw - white) / span;
    *fraction = value < 0 ? 0 : (value > 1 ? 1 : value);
    return true;
}
bool bench_echo_distance(uint32_t width_us, float *metres) {
    /* Representative HC-SR04 principle: microseconds / 58 = centimetres. */
    if (width_us < 116u || width_us > 23200u) return false; /* Approx. 2cm to 4m. */
    *metres = width_us / 5800.0f;
    return true;
}
unsigned bench_servo_pulse(unsigned angle_deg) {
    if (angle_deg > 180) angle_deg = 180;
    return BENCH_SERVO_MIN_US + angle_deg * (BENCH_SERVO_MAX_US - BENCH_SERVO_MIN_US) / 180u;
}

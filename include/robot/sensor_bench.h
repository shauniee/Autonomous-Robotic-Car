#ifndef ROBOT_SENSOR_BENCH_H
#define ROBOT_SENSOR_BENCH_H
#include <stddef.h>
#include "robot/types.h"
/* Pin allocation from CSC01 Design Review.docx, read only. */
#define BENCH_IR_GPIO 26u
#define BENCH_SDA_GPIO 4u
#define BENCH_SCL_GPIO 5u
#define BENCH_ECHO_GPIO 2u
#define BENCH_TRIG_GPIO 3u
#define BENCH_SERVO_GPIO 12u
#define BENCH_ACCEL_ADDR 0x19u
#define BENCH_MAG_ADDR 0x1eu
#define BENCH_SAMPLE_MAX_AGE_MS 250u
#define BENCH_RANGE_MAX_AGE_MS 1500u
#define BENCH_ECHO_TIMEOUT_US 30000u
#define BENCH_TRIGGER_GAP_US 65000u
#define BENCH_SERVO_SETTLE_MS 300u
/* Conservative bench sweep and pulse endpoints; calibrate the installed SG90. */
#define BENCH_SERVO_MIN_US 1000u
#define BENCH_SERVO_MAX_US 2000u
#define BENCH_SCAN_COUNT 3u

typedef struct { SampleStatus status; float x, y, z; int16_t raw[3]; } BenchVector;
typedef struct { SampleStatus status; unsigned angle_deg; uint32_t echo_us; float distance_m; } BenchRange;
typedef struct {
    SampleStatus ir_status;
    uint16_t ir_raw, ir_black, ir_white;
    bool ir_calibrated;
    float ir_black_fraction;
    bool accel_configured, mag_configured;
    BenchVector accel, magnetic;
    bool attitude_valid;
    float pitch_deg, roll_deg;
    uint32_t i2c_errors;
    bool servo_enabled, scan_enabled;
    unsigned servo_command_deg, servo_pulse_us;
    uint32_t servo_command_ms;
    BenchRange range, forward, scan[BENCH_SCAN_COUNT];
    uint32_t sonar_attempts, sonar_timeouts;
} SensorBench;

void sensor_bench_init(SensorBench *s);
void sensor_bench_update(SensorBench *s, uint32_t now_ms);
void sensor_bench_command(SensorBench *s, int key, uint32_t now_ms);
size_t sensor_bench_encode(char *out, size_t capacity, const SensorBench *s, uint32_t now_ms);
/* Pure conversion helpers, used in the real driver and host tests. */
void bench_decode_accel(const uint8_t bytes[6], BenchVector *v);
bool bench_decode_magnetic(const uint8_t bytes[6], BenchVector *v);
bool bench_tilt(float x, float y, float z, float *pitch, float *roll);
bool bench_ir_fraction(uint16_t raw, uint16_t black, uint16_t white, float *fraction);
bool bench_echo_distance(uint32_t width_us, float *metres);
unsigned bench_servo_pulse(unsigned angle_deg);
#endif

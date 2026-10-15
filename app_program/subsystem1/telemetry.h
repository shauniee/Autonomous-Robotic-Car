#ifndef S1_TELEMETRY_H
#define S1_TELEMETRY_H
#include <stdbool.h>
#include <stdint.h>

#define S1_TELEMETRY_PERIOD_MS 500u
#define S1_TELEMETRY_PAYLOAD_MAX 1536u

/* Portable across kernel/network headers. Decimal values use thousandths;
   duties are requests, not measured speed or proof that motors are moving. */
typedef struct {
    uint32_t sequence, observed_ms, sampled_ms;
    uint16_t raw_left, raw_right, black_left, black_right, white_left, white_right;
    int16_t left_fraction, right_fraction, error, left_duty, right_duty;
    bool have_sample, acquired, black_set, white_set, reading_valid;
    bool motion_enabled, test_inhibited;
    char state[16], line[16], reason[40], capture[32];
    uint32_t run_epoch, action_id, completed_actions;
    bool configured, safety_latched;
    uint32_t motion_ms, motion_sequence, left_count, right_count;
    int32_t left_mm, right_mm, left_mmps, right_mmps, x_mm, y_mm, yaw_mdeg;
    int16_t output_left, output_right;
    unsigned motion_status, motion_reason;
    bool motion_valid, stationary;
    uint32_t terrain_ms, terrain_sequence, magnetic_ms;
    int32_t accel_mg[3], magnetic_milli_gauss[3];
    int32_t pitch_mdeg, roll_mdeg, turn_mdeg_s, height_mm, max_height_mm;
    unsigned terrain_state, movement;
    bool terrain_valid, terrain_calibrated, height_valid, magnetic_valid;
    uint32_t front_ms, profile_ms, scan_id;
    int32_t front_mm, closest_mm, width_mm, left_gap_mm, right_gap_mm;
    unsigned scan_status;
    bool front_valid, profile_valid;
    char symbol[2];
    unsigned navigation, orientation;
    uint32_t mapping_revision, scanner_epoch, barcode_faults, mapping_request_id;
    unsigned mapping_result;
} S1Telemetry;

/* Both use TMO_POL. Before init, or when busy, return false immediately.
   Copy owns its destination; updates never affect control or acquisition time. */
bool s1_telemetry_update(const S1Telemetry *snapshot);
bool s1_telemetry_copy(S1Telemetry *snapshot);
/* Pure bounded JSON encoder. Failure returns zero and clears destination. */
unsigned s1_telemetry_encode(const S1Telemetry *snapshot, uint32_t now,
                             char *destination, unsigned capacity);
#endif

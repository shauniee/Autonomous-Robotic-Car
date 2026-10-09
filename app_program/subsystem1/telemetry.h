#ifndef S1_TELEMETRY_H
#define S1_TELEMETRY_H
#include <stdbool.h>
#include <stdint.h>

#define S1_TELEMETRY_PERIOD_MS 500u
#define S1_TELEMETRY_PAYLOAD_MAX 1536u

/* Portable across kernel/network headers. Decimal values use thousandths;
   duties are requests, not measured speed or proof that motors are moving. */
typedef struct {
    uint32_t sequence, observed_ms, sampled_ms, ir_sequence;
    uint32_t motion_serviced_ms;
    uint16_t raw_left, raw_right, black_left, black_right, white_left, white_right;
    int16_t left_fraction, right_fraction, error, left_duty, right_duty;
    int16_t applied_left, applied_right;
    bool have_sample, acquired, black_set, white_set, reading_valid;
    bool motion_enabled, test_inhibited;
    bool motion_available, motion_stopped, motion_braked;
    bool velocity_mode, speed_target_valid, encoder_valid, encoder_calibrated;
    bool barcode_available;
    int32_t speed_left_milli, speed_right_milli;
    int32_t target_left_milli, target_right_milli;
    uint32_t encoder_left_count, encoder_right_count, encoder_errors;
    uint32_t navigation_sampled_ms, action_id, action_state;
    uint32_t barcode_serviced_ms, barcode_sequence, barcode_queued;
    uint32_t barcode_accepted, barcode_rejected, barcode_discarded;
    char barcode_state[12], barcode_result[24], barcode_symbol[2];
    char state[16], line[16], reason[40], capture[32];
} S1Telemetry;

/* Both use TMO_POL. Before init, or when busy, return false immediately.
   Copy owns its destination; updates never affect control or acquisition time. */
bool s1_telemetry_update(const S1Telemetry *snapshot);
bool s1_telemetry_copy(S1Telemetry *snapshot);
/* Kernel service only: collect task-context snapshots without waiting.
   Required IR/controller copies unavailable -> skip this publication. */
bool s1_telemetry_refresh(uint32_t now);
/* Pure bounded JSON encoder. Failure returns zero and clears destination. */
unsigned s1_telemetry_encode(const S1Telemetry *snapshot, uint32_t now,
                             char *destination, unsigned capacity);
#endif

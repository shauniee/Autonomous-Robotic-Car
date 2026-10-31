#ifndef RECOVERY_CONTROL_H
#define RECOVERY_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    RECOVERY_WAIT,
    RECOVERY_SEARCH,
    RECOVERY_DONE,
    RECOVERY_FAILED
} recovery_phase_t;
typedef struct
{
    uint32_t origin[2], started_ms;
    uint8_t phase : 3;
    uint8_t right : 1;
    uint8_t turning : 1;
    int16_t turn_degrees : 10; /* held relative target, -360..360 degrees */
} recovery_control_t;
typedef struct
{
    uint32_t now_ms, count[2], sampled_ms;
    bool functional, visible, hazard, cancel;
    bool stationary, motion_done, motion_failed;
} recovery_input_t;
typedef struct
{
    recovery_phase_t phase;
    bool turn, new_turn;
    int16_t turn_degrees;
    int16_t heading_cdeg;
} recovery_output_t;

void recovery_control_begin(recovery_control_t *control, const recovery_input_t *input);
void recovery_control_step(recovery_control_t *control, const recovery_input_t *input,
    recovery_output_t *output);

#endif /* RECOVERY_CONTROL_H */

#ifndef RECOVERY_CONTROL_H
#define RECOVERY_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

typedef enum { RECOVERY_SEARCH, RECOVERY_ALIGN, RECOVERY_DONE, RECOVERY_FAILED } recovery_phase_t;
typedef struct
{
    uint32_t origin[2], started_ms, sampled_ms;
    uint8_t phase : 2;
    uint8_t leg : 3;
    uint8_t stable : 3;
} recovery_control_t;
typedef struct
{
    uint32_t now_ms, count[2], sampled_ms;
    bool functional, visible, hazard, cancel;
    int8_t error_q, steering_q;
} recovery_input_t;
typedef struct
{
    recovery_phase_t phase;
    int8_t steering_q;
    int16_t heading_cdeg;
} recovery_output_t;

void recovery_control_begin(recovery_control_t *control, const recovery_input_t *input);
void recovery_control_step(recovery_control_t *control, const recovery_input_t *input,
    recovery_output_t *output);

#endif /* RECOVERY_CONTROL_H */

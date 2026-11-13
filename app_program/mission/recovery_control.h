#ifndef RECOVERY_CONTROL_H
#define RECOVERY_CONTROL_H

#include <stdbool.h>
#include <stdint.h>
#include "../motion/motion.h"

typedef enum
{
    RECOVERY_WAIT,
    RECOVERY_SEARCH,
    RECOVERY_DONE,
    RECOVERY_FAILED
} recovery_phase_t;
typedef struct
{
    enc_count_t origin[2];
    uint32_t started_ms;
    uint8_t phase : 2; /* recovery_phase_t */
    uint8_t right : 1;
    uint8_t turning : 1;
    uint8_t seen : 1; /* a sensor has been black since begin */
    int16_t turn_degrees : 10; /* held relative target, -360..360 degrees */
} recovery_control_t;
typedef struct
{
    uint32_t now_ms;
    uint32_t sampled_ms;
    enc_count_t count[2];
    bool functional : 1;
    bool visible : 1;
    bool black : 1; /* a sensor sees the line now */
    bool hazard : 1;
    bool cancel : 1;
    bool stationary : 1;
    bool motion_done : 1;
    bool motion_failed : 1;
} recovery_input_t;
typedef struct
{
    int16_t heading_cdeg;
    int16_t turn_degrees; /* -360..360; signed type (BARR-C 5.3.a) */
    uint8_t phase : 2; /* recovery_phase_t */
    bool turn : 1;
    bool new_turn : 1;
} recovery_output_t;

_Static_assert(RECOVERY_FAILED < 4, "recovery phase fits 2 bits");

void recovery_control_begin(recovery_control_t *control,
    const recovery_input_t *input);
void recovery_control_step(recovery_control_t *control,
    const recovery_input_t *input,
    recovery_output_t *output);

#endif /* RECOVERY_CONTROL_H */

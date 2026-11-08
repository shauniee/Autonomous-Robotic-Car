#ifndef BYPASS_CONTROL_H
#define BYPASS_CONTROL_H

#include <stdbool.h>
#include <stdint.h>
#include "../motion/motion.h"
#include "../obstacle/obstacle.h"

typedef enum
{
    BYPASS_ROUTE_OUT, BYPASS_ROUTE_OFFSET, BYPASS_ROUTE_PARALLEL,
    BYPASS_ROUTE_PASS, BYPASS_ROUTE_REAR, BYPASS_ROUTE_IN,
    BYPASS_ROUTE_RETURN, BYPASS_ROUTE_DONE, BYPASS_ROUTE_FAILED
} bypass_route_t;
typedef enum
{
    BYPASS_WORK_TURN, BYPASS_WORK_DRIVE, BYPASS_WORK_LOOK, BYPASS_WORK_STOP
} bypass_work_t;
typedef struct
{
    enc_count_t origin[2], previous[2];
    uint32_t started_ms, phase_ms, action_ms;
    uint32_t sensor_sequence;
    /* Fill spare bits in the Q8 storage words without unaligned accesses. */
    int32_t forward_q8 : 21;
    uint8_t route : 4; /* bypass_route_t */
    uint8_t work : 2; /* bypass_work_t */
    uint8_t left_side : 1;
    uint8_t edge_open : 1;
    uint8_t close_samples : 2;
    int32_t left_q8 : 21;
    uint8_t edge_samples : 3;
    uint8_t action_id : 6;
    int32_t rear_clear_q8 : 21;
    int32_t edge_forward_q8 : 21;
    uint32_t edge_counts : 17; /* <= 2 x BYPASS_TRAVEL_MM of edges, 93192 */
    uint16_t half_width_mm : 9; /* half the 10-bit profile width */
    /* Signed: smallest signed type (BARR-C 5.3.a). Profile coordinates are
       +/-1023 mm; computed geometry has no fixed bound. */
    int16_t centre_mm;
    int16_t obstacle_front_mm;
    int16_t heading_cdeg;
    int16_t offset_mm;
    int16_t action_target;
    uint16_t flank_mm;
} bypass_control_t;
typedef struct
{
    uint32_t now_ms;
    uint32_t front_ms;
    uint32_t front_sequence;
    uint32_t side_ms;
    uint32_t side_sequence;
    uint32_t line_ms;
    enc_count_t count[2];
    enc_count_t front_count[2];
    uint16_t front_mm : 13; /* echo range */
    uint16_t side_mm : 13;
    bool stationary : 1;
    bool cancel : 1;
    bool functional : 1;
    bool front_valid : 1;
    bool side_valid : 1;
    bool line_functional : 1;
    bool line_black : 1; /* a sensor sees the line now */
    uint8_t motion_status : 2; /* rd_motion_status_t */
    uint8_t motion_action : 6;
} bypass_input_t;
typedef struct
{
    MotionCommand command;
    int8_t side_angle_deg; /* +/-SERVO_LOOK_DEG */
    uint8_t route : 4; /* bypass_route_t */
    bool side_watch : 1;
} bypass_output_t;

_Static_assert(BYPASS_ROUTE_FAILED < 16 && BYPASS_WORK_STOP < 4,
    "bypass enums fit their fields");
_Static_assert((uint32_t)(2.0f * BYPASS_TRAVEL_MM * ENC_COUNTS_PER_MM) <
    (1u << 17), "bypass edge budget fits 17 bits");

/* A supported measured profile and stationary input are prerequisites.
   Planning width is supplied separately from the reported measurement. */
/* Millimetres to reverse so a side with room can pivot, or 0 if none
   applies. */
uint8_t bypass_control_backup_mm(const ObstacleProfile *profile,
    uint16_t planning_width_mm);
bool bypass_control_can_begin(const bypass_input_t *input,
    const ObstacleProfile *profile, uint16_t planning_width_mm);
bool bypass_control_begin(bypass_control_t *control, const bypass_input_t *input,
    const ObstacleProfile *profile, uint16_t planning_width_mm, uint8_t *action_id);
void bypass_control_step(bypass_control_t *control, const bypass_input_t *input,
    uint8_t *action_id, bypass_output_t *output);

#endif /* BYPASS_CONTROL_H */

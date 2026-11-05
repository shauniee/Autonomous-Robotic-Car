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
    uint32_t sensor_sequence, edge_counts;
    int32_t forward_q8 : 21;
    int32_t left_q8 : 21;
    int32_t rear_clear_q8 : 21;
    int32_t edge_forward_q8 : 21;
    int16_t heading_cdeg, offset_mm, centre_mm, obstacle_front_mm;
    int16_t action_target;
    uint16_t depth_mm, half_width_mm, flank_mm;
    uint8_t action_id;
    uint8_t spare_mm; /* room left to step out; 255 is six steps */
    uint8_t route : 4;
    uint8_t work : 3;
    uint8_t left_side : 1;
    uint8_t edge_open : 1;
    uint8_t edge_samples : 3;
    uint8_t close_samples : 2;
} bypass_control_t;
typedef struct
{
    uint32_t now_ms, count[2];
    bool stationary, cancel, functional;
    uint32_t front_ms, front_sequence, front_count[2], front_mm;
    uint32_t side_ms, side_sequence, side_mm;
    bool front_valid, side_valid;
    uint32_t line_ms;
    bool line_functional, line_black; /* black: a sensor sees the line now */
    rd_motion_status_t motion_status;
    uint8_t motion_action;
} bypass_input_t;
typedef struct
{
    MotionCommand command;
    bool side_watch, depth_event;
    int16_t side_angle_deg;
    uint16_t depth_mm;
    bypass_route_t route;
} bypass_output_t;

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

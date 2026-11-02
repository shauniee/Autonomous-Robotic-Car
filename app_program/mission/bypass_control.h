#ifndef BYPASS_CONTROL_H
#define BYPASS_CONTROL_H

#include <stdbool.h>
#include <stdint.h>
#include "../motion/motion_control.h"
#include "../obstacle/obstacle_profile.h"

typedef enum
{
    BYPASS_ROUTE_OUT, BYPASS_ROUTE_OFFSET, BYPASS_ROUTE_PARALLEL,
    BYPASS_ROUTE_PASS, BYPASS_ROUTE_REAR, BYPASS_ROUTE_IN,
    BYPASS_ROUTE_RETURN, BYPASS_ROUTE_FORWARD, BYPASS_ROUTE_LINE,
    BYPASS_ROUTE_DONE, BYPASS_ROUTE_FAILED
} bypass_route_t;
typedef enum
{
    BYPASS_WORK_TURN, BYPASS_WORK_SIDE, BYPASS_WORK_FRONT,
    BYPASS_WORK_DRIVE, BYPASS_WORK_LINE
} bypass_work_t;
typedef struct
{
    uint32_t origin[2], previous[2];
    uint32_t started_ms, phase_ms, action_ms, line_started_ms;
    uint32_t sensor_sequence, line_sampled_ms, edge_counts;
    uint32_t chunk_origin[2];
    int32_t forward_q8 : 21;
    int32_t left_q8 : 21;
    int32_t rear_clear_q8 : 21;
    int32_t edge_forward_q8 : 21;
    int16_t heading_cdeg, offset_mm, pass_start_mm, obstacle_front_mm;
    int16_t action_target;
    uint16_t baseline_mm, depth_mm;
    uint8_t action_id;
    uint8_t route : 4;
    uint8_t work : 3;
    uint8_t left_side : 1;
    uint8_t baseline_valid : 1;
    uint8_t edge_streak : 3;
    uint8_t attempts : 2;
    uint8_t visible_streak : 3;
    uint8_t line_seen : 1; /* a sensor has been black since the line phase began */
} bypass_control_t;
typedef struct
{
    uint32_t now_ms, count[2];
    bool stationary, cancel, functional;
    uint32_t front_ms, front_sequence, front_count[2], front_mm;
    uint32_t side_ms, side_sequence, side_mm;
    bool front_valid, side_valid;
    uint32_t line_ms;
    bool line_functional, line_visible, line_black; /* black: a sensor sees the line now */
    int8_t line_error_q, line_steering_q;
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
bool bypass_control_begin(bypass_control_t *control, const bypass_input_t *input,
    const ObstacleProfile *profile, uint16_t planning_width_mm, uint8_t *action_id);
void bypass_control_step(bypass_control_t *control, const bypass_input_t *input,
    uint8_t *action_id, bypass_output_t *output);

#endif /* BYPASS_CONTROL_H */

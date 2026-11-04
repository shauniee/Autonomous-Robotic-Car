#include "bypass_control.h"
#include "../common/robot_time_math.h"
#include <math.h>
#include <stdlib.h>

#define BYPASS_RAD_PER_CDEG 0.000174532925f
#define BYPASS_RAD_PER_DEG 0.0174532925f
#define BYPASS_Q8 256.0f

static uint8_t next_id(uint8_t *id)
{
    *id = (uint8_t)(*id >= rd_mask(RD_MOTION_ACTION_BITS) || *id < 1u ? 2u : *id + 1u);
    return *id;
}

static float forward_mm(const bypass_control_t *control)
{
    return (float)control->forward_q8 / BYPASS_Q8;
}

static float left_mm(const bypass_control_t *control)
{
    return (float)control->left_q8 / BYPASS_Q8;
}

static void fail(bypass_control_t *control)
{
    control->route = BYPASS_ROUTE_FAILED;
}

static bool pose_step(bypass_control_t *control, const bypass_input_t *input)
{
    int32_t delta[2];
    uint64_t edges = 0u;
    for (unsigned wheel = 0u; wheel < 2u; ++wheel)
    {
        delta[wheel] = enc_travel_signed(input->count[wheel], control->previous[wheel]);
        edges += delta[wheel] < 0 ? (uint64_t)(-(int64_t)delta[wheel]) : (uint32_t)delta[wheel];
        control->previous[wheel] = input->count[wheel];
    }
    uint32_t maximum = (uint32_t)(2.0f * BYPASS_TRAVEL_MM * ENC_COUNTS_PER_MM);
    if (control->edge_counts > maximum || edges > maximum - control->edge_counts)
    {
        return false;
    }
    control->edge_counts += (uint32_t)edges;
    /* Common forward travel may exceed a wheel's signed half range. */
    int32_t difference = enc_travel_signed(input->count[1] - input->count[0],
        control->origin[1] - control->origin[0]);
    float heading = enc_heading_deg(0, difference) * 100.0f;
    if (fabsf(heading) > 18000.0f)
    {
        return false;
    }
    float middle = ((float)control->heading_cdeg + heading) * 0.5f * BYPASS_RAD_PER_CDEG;
    float distance = ((float)delta[0] + delta[1]) * 0.5f / ENC_COUNTS_PER_MM;
    float x = control->forward_q8 + distance * cosf(middle) * BYPASS_Q8;
    float y = control->left_q8 + distance * sinf(middle) * BYPASS_Q8;
    if (fabsf(x) > BYPASS_TRAVEL_MM * BYPASS_Q8 ||
        fabsf(y) > BYPASS_TRAVEL_MM * BYPASS_Q8)
    {
        return false;
    }
    control->forward_q8 = (int32_t)lroundf(x);
    control->left_q8 = (int32_t)lroundf(y);
    control->heading_cdeg = (int16_t)lroundf(heading);
    return true;
}

static void turn(bypass_control_t *control, const bypass_input_t *input,
    bypass_route_t route, int target_heading, uint8_t *id)
{
    control->route = route;
    control->work = BYPASS_WORK_TURN;
    control->action_target = (int16_t)lroundf(target_heading -
        (float)control->heading_cdeg / 100.0f);
    control->action_id = next_id(id);
    control->action_ms = input->now_ms;
}

static int expected_heading(const bypass_control_t *control)
{
    int side = control->left_side ? 1 : -1;
    if (control->route == BYPASS_ROUTE_OFFSET || control->route == BYPASS_ROUTE_OUT)
    {
        return side * control->out_deg;
    }
    if (control->route == BYPASS_ROUTE_RETURN || control->route == BYPASS_ROUTE_IN)
    {
        return -side * 90;
    }
    return 0;
}

/* How far ahead of the axle a pivot reaches. A right pivot swings the barcode
   arm across the front; a left pivot swings the shorter front-right corner and
   only has to clear the arm where it starts. */
static float pivot_reach_mm(bool left_turn)
{
    float reach = left_turn ?
        fmaxf(hypotf(BYPASS_BODY_NOSE_MM, BYPASS_BODY_NOSE_RIGHT_MM),
            BYPASS_BODY_ARM_FORWARD_MM) :
        hypotf(BYPASS_BODY_ARM_FORWARD_MM, BYPASS_BODY_LEFT_MM);
    return fmaxf(reach, hypotf(BYPASS_BODY_REAR_MM, BYPASS_BODY_LEFT_MM)) +
        BYPASS_CLEARANCE_MM;
}

/* The side of the car that faces the box while passing on the given side. */
static float facing_half_mm(bool pass_left)
{
    return pass_left ? BYPASS_BODY_RIGHT_MM : BYPASS_BODY_LEFT_MM;
}

static bool has_clearance(const ObstacleProfile *profile, bool left)
{
    uint16_t clearance = left ? profile->left_clearance_mm : profile->right_clearance_mm;
    return clearance >= fmaxf(MISSION_BYPASS_MIN_CLEAR_MM,
        BYPASS_BODY_LEFT_MM + BYPASS_BODY_RIGHT_MM +
        2.0f * BYPASS_CLEARANCE_MM);
}

/* Least pivot that keeps the facing side BYPASS_SAFE_GAP_MM off the measured
   box corner along a straight leg: F sin(a) - E cos(a) = d, with the box
   corner at (F, E). The leg ends once a straight run alongside the box would
   hold the same gap. */
static bool plan_side(const ObstacleProfile *profile, bool left, float *offset,
    uint8_t *out_deg)
{
    float forward = profile->closest_mm;
    float edge = (left ? profile->obstacle_left_mm :
        -profile->obstacle_left_mm) + profile->width_mm * 0.5f;
    float gap = facing_half_mm(left) + BYPASS_SAFE_GAP_MM;
    float reach = hypotf(forward, edge);
    *offset = edge + gap;
    if (*offset <= 0.0f || *offset > BYPASS_MAX_OFFSET_MM || gap >= reach ||
        *offset > BYPASS_LEG_MAX_MM)
    {
        return false;
    }
    float angle = fmaxf(atan2f(edge, forward) + asinf(gap / reach),
        asinf(*offset / BYPASS_LEG_MAX_MM)) / BYPASS_RAD_PER_DEG;
    if (angle > 90.0f)
    {
        return false;
    }
    *out_deg = (uint8_t)ceilf(angle);
    return true;
}

static bool side_usable(const ObstacleProfile *profile, bool left,
    float *offset, uint8_t *out_deg)
{
    return has_clearance(profile, left) &&
        profile->closest_mm > pivot_reach_mm(left) &&
        plan_side(profile, left, offset, out_deg);
}

uint8_t bypass_control_backup_mm(const ObstacleProfile *profile,
    uint16_t planning_width_mm)
{
    (void)planning_width_mm;
    float least = BYPASS_BACKUP_MAX_MM + 1.0f;
    for (int side = 0; side < 2; ++side)
    {
        if (has_clearance(profile, side != 0))
        {
            least = fminf(least, pivot_reach_mm(side != 0) +
                BYPASS_BACKUP_MARGIN_MM - profile->closest_mm);
        }
    }
    return least > 0.0f && least <= BYPASS_BACKUP_MAX_MM ?
        (uint8_t)ceilf(least) : 0u;
}

/* No echo is not a hazard on a bounded leg; a close echo or a silent sensor
   is. */
static bool front_clear(const bypass_input_t *input)
{
    return robot_time_elapsed(input->now_ms, input->front_ms) <=
        BYPASS_FRONT_MAX_AGE_MS &&
        (!input->front_valid || input->front_mm >
            BYPASS_BODY_ARM_FORWARD_MM - OBSTACLE_SONAR_FORWARD_MM +
            BYPASS_CLEARANCE_MM);
}

static bool heading_held(const bypass_control_t *control)
{
    return fabsf((float)control->heading_cdeg / 100.0f -
        expected_heading(control)) <= BYPASS_RETURN_HEADING_DEG;
}

bool bypass_control_can_begin(const bypass_input_t *input,
    const ObstacleProfile *profile, uint16_t planning_width_mm)
{
    float offset = 0.0f;
    uint8_t deg = 0u;
    return input != NULL && profile != NULL && input->stationary &&
        input->functional && profile->valid &&
        planning_width_mm >= OBSTACLE_EXPECTED_MAX_WIDTH_MM &&
        (side_usable(profile, true, &offset, &deg) ||
            side_usable(profile, false, &offset, &deg));
}

bool bypass_control_begin(bypass_control_t *control,
    const bypass_input_t *input, const ObstacleProfile *profile,
    uint16_t planning_width_mm, uint8_t *action_id)
{
    if (control == NULL || action_id == NULL ||
        !bypass_control_can_begin(input, profile, planning_width_mm))
    {
        return false;
    }
    float left_offset = 0.0f, right_offset = 0.0f;
    uint8_t left_deg = 0u, right_deg = 0u;
    bool can_left = side_usable(profile, true, &left_offset, &left_deg);
    bool can_right = side_usable(profile, false, &right_offset, &right_deg);
    bool left = can_left && (!can_right ||
        profile->left_clearance_mm >= profile->right_clearance_mm);
    float offset = left ? left_offset : right_offset;
    uint8_t out_deg = left ? left_deg : right_deg;
    *control = (bypass_control_t){.origin = {input->count[0], input->count[1]},
        .previous = {input->count[0], input->count[1]},
        .started_ms = input->now_ms,
        .left_side = left,
        .offset_mm = (int16_t)lroundf(left ? offset : -offset),
        .centre_mm = profile->obstacle_left_mm,
        .half_width_mm = (uint16_t)(profile->width_mm / 2u),
        .out_deg = out_deg,
        .obstacle_front_mm = profile->obstacle_forward_mm};
    turn(control, input, BYPASS_ROUTE_OUT, left ? out_deg : -(int)out_deg,
        action_id);
    return true;
}

static void drive(bypass_control_t *control, const bypass_input_t *input,
    bypass_route_t route, float distance_mm, uint8_t *id)
{
    control->route = route;
    control->work = BYPASS_WORK_DRIVE;
    control->action_target = (int16_t)ceilf(distance_mm);
    control->action_id = next_id(id);
    control->action_ms = input->now_ms;
}

/* The side ray meets the box's side until past its rear. A reading beyond the
   box's centreline, or none, has to last BYPASS_EDGE_MM of travel to count as
   clear. */
static void side_observation(bypass_control_t *control, const bypass_input_t *input,
    bypass_output_t *output)
{
    float side = control->left_side ? 1.0f : -1.0f;
    float angle = SERVO_LOOK_DEG * BYPASS_RAD_PER_DEG;
    float to_centre = side * (left_mm(control) - control->centre_mm);
    float across = input->side_mm * sinf(angle);
    if (input->side_valid && across < facing_half_mm(control->left_side))
    {
        if (++control->close_samples >= BYPASS_CLOSE_SAMPLES)
        {
            fail(control);
        }
        return;
    }
    control->close_samples = 0u;
    if (input->side_valid && across <= to_centre)
    {
        control->edge_open = 0u;
        control->flank_mm = (uint16_t)lroundf(across);
        return;
    }
    if (!control->edge_open)
    {
        control->edge_open = 1u;
        control->edge_samples = 0u;
        control->edge_forward_q8 = control->forward_q8;
    }
    if (control->edge_samples < BYPASS_EDGE_SAMPLES)
    {
        ++control->edge_samples;
    }
    if (control->edge_samples < BYPASS_EDGE_SAMPLES ||
        forward_mm(control) - (float)control->edge_forward_q8 / BYPASS_Q8 <
        BYPASS_EDGE_MM)
    {
        return;
    }
    /* The ray left the box here, so its rear is no further than where the ray
       met the box's side: at the measured range if it ever saw that side, else
       at the scan's edge. */
    float to_side = control->flank_mm != 0u ? (float)control->flank_mm :
        fmaxf(to_centre - control->half_width_mm, 0.0f);
    float rear = (float)control->edge_forward_q8 / BYPASS_Q8 +
        OBSTACLE_SONAR_FORWARD_MM + to_side / tanf(angle);
    control->rear_clear_q8 = (int32_t)lroundf(
        (rear + facing_half_mm(control->left_side) + BYPASS_SAFE_GAP_MM) *
        BYPASS_Q8);
    control->route = BYPASS_ROUTE_REAR;
    float depth = fmaxf(0.0f, rear - control->obstacle_front_mm);
    control->depth_mm = (uint16_t)lroundf(fminf(depth, 65535.0f));
    output->depth_event = true;
    output->depth_mm = control->depth_mm;
}

void bypass_control_step(bypass_control_t *control, const bypass_input_t *input,
    uint8_t *action_id, bypass_output_t *output)
{
    *output = (bypass_output_t){.command = motion_command_make(MOTION_MODE_STOP,
        0.0f, 0.0f, 0, 0u, input->now_ms), .route = BYPASS_ROUTE_FAILED};
    if (input->cancel || !input->functional ||
        robot_time_elapsed(input->now_ms, control->started_ms) >= MISSION_BYPASS_TIMEOUT_MS ||
        !pose_step(control, input) || control->route > BYPASS_ROUTE_FAILED)
    {
        fail(control);
    }
    if (control->route == BYPASS_ROUTE_RETURN)
    {
        if (!input->line_functional ||
            robot_time_elapsed(input->now_ms, input->line_ms) > IR_MAX_GAP_MS)
        {
            fail(control);
        }
        else if (input->line_black)
        {
            /* The line itself ends the return, so wheel drift over the bypass
               does not matter. Mission crosses it like a junction turn. */
            control->route = BYPASS_ROUTE_DONE;
        }
    }
    if (control->route == BYPASS_ROUTE_DONE || control->route == BYPASS_ROUTE_FAILED)
    {
        output->route = (bypass_route_t)control->route;
        return;
    }
    int side = control->left_side ? 1 : -1;
    bool finished = input->motion_action == control->action_id &&
        input->motion_status == RD_MOTION_DONE;
    bool action_failed = input->motion_action == control->action_id &&
        input->motion_status == RD_MOTION_FAILED;
    if (control->work == BYPASS_WORK_TURN)
    {
        bool swept_hazard = control->route != BYPASS_ROUTE_PARALLEL &&
            input->front_valid &&
            robot_time_elapsed(input->now_ms, input->front_ms) <= BYPASS_FRONT_MAX_AGE_MS &&
            input->front_mm + OBSTACLE_SONAR_FORWARD_MM <=
                pivot_reach_mm(control->action_target > 0);
        if (swept_hazard || action_failed ||
            robot_time_elapsed(input->now_ms, control->action_ms) >=
            MOTION_ACTION_TIMEOUT_MS)
        {
            fail(control);
        }
        else if (finished)
        {
            if (!heading_held(control))
            {
                fail(control);
            }
            else if (control->route == BYPASS_ROUTE_OUT)
            {
                /* The pivot's real heading sets the leg, so its error does not
                   add up. */
                float heading = (float)control->heading_cdeg *
                    BYPASS_RAD_PER_CDEG;
                drive(control, input, BYPASS_ROUTE_OFFSET,
                    fabsf(((float)control->offset_mm - left_mm(control)) /
                    sinf(heading)), action_id);
            }
            else if (control->route == BYPASS_ROUTE_PARALLEL)
            {
                control->route = BYPASS_ROUTE_PASS;
                control->work = BYPASS_WORK_LOOK;
                control->phase_ms = input->now_ms;
                control->sensor_sequence = input->side_sequence;
            }
            else if (control->route == BYPASS_ROUTE_IN)
            {
                drive(control, input, BYPASS_ROUTE_RETURN,
                    fabsf(left_mm(control)), action_id);
            }
            else
            {
                fail(control);
            }
        }
    }
    else if (control->work == BYPASS_WORK_LOOK)
    {
        /* The servo has settled once the first reading from the side
           arrives. */
        if (robot_time_elapsed(input->now_ms, control->phase_ms) >=
            BYPASS_SENSE_TIMEOUT_MS)
        {
            fail(control);
        }
        else if (input->side_sequence != control->sensor_sequence)
        {
            control->sensor_sequence = input->side_sequence;
            drive(control, input, BYPASS_ROUTE_PASS, BYPASS_LEG_MAX_MM,
                action_id);
        }
    }
    else if (control->work == BYPASS_WORK_STOP)
    {
        if (input->stationary)
        {
            turn(control, input, BYPASS_ROUTE_IN, -side * 90, action_id);
        }
    }
    else if (control->work == BYPASS_WORK_DRIVE)
    {
        bool passing = control->route == BYPASS_ROUTE_PASS ||
            control->route == BYPASS_ROUTE_REAR;
        if (action_failed || !heading_held(control) ||
            (!passing && !front_clear(input)) ||
            robot_time_elapsed(input->now_ms, control->action_ms) >=
            MOTION_ACTION_TIMEOUT_MS)
        {
            fail(control);
        }
        else if (passing)
        {
            if (control->route == BYPASS_ROUTE_PASS &&
                input->side_sequence != control->sensor_sequence)
            {
                control->sensor_sequence = input->side_sequence;
                side_observation(control, input, output);
                if (control->route == BYPASS_ROUTE_REAR)
                {
                    /* Slow to rest at the point that clears the box, or as soon
                       after it as the stop allows. */
                    drive(control, input, BYPASS_ROUTE_REAR, fmaxf(
                        (float)control->rear_clear_q8 / BYPASS_Q8 - forward_mm(control),
                        motion_stop_distance_mm(BYPASS_SPEED_MMPS)), action_id);
                    finished = false;
                }
            }
            if (finished && control->route == BYPASS_ROUTE_REAR)
            {
                control->work = BYPASS_WORK_STOP;
            }
            else if (finished)
            {
                /* the leg ran out with the box still beside the car */
                fail(control);
            }
        }
        else if (finished)
        {
            if (control->route == BYPASS_ROUTE_OFFSET)
            {
                turn(control, input, BYPASS_ROUTE_PARALLEL, 0, action_id);
            }
            else
            {
                fail(control); /* RETURN: the sensors passed the line unseen */
            }
        }
    }
    else
    {
        fail(control);
    }
    output->route = (bypass_route_t)control->route;
    if (control->route == BYPASS_ROUTE_FAILED || control->route == BYPASS_ROUTE_DONE)
    {
        output->side_watch = false;
        return;
    }
    /* Start the look during the turn to parallel, so the car barely waits for
       the servo. */
    output->side_watch = control->route == BYPASS_ROUTE_PARALLEL ||
        control->route == BYPASS_ROUTE_PASS ||
        control->route == BYPASS_ROUTE_REAR;
    output->side_angle_deg = (int16_t)(-side * SERVO_LOOK_DEG);
    if (control->work == BYPASS_WORK_TURN)
    {
        output->command = motion_command_make(MOTION_MODE_TURN, BYPASS_TURN_SPEED_MMPS,
            control->action_target, 0, control->action_id, input->now_ms);
    }
    else if (control->work == BYPASS_WORK_DRIVE)
    {
        output->command = motion_command_make(MOTION_MODE_DISTANCE, BYPASS_SPEED_MMPS,
            control->action_target, control->route == BYPASS_ROUTE_REAR ? MOTION_TURN_RAMPED : 0,
            control->action_id, input->now_ms);
    }
}

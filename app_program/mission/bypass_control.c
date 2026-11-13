#include "bypass_control.h"
#include "../common/robot_time_math.h"
#include <math.h>
#include <stdlib.h>

#define BYPASS_RAD_PER_CDEG 0.000174532925f
#define BYPASS_RAD_PER_DEG 0.0174532925f
#define BYPASS_Q8 256.0f

static uint8_t next_id(uint8_t *id)
{
    *id = (uint8_t)(*id >= rd_mask(RD_MOTION_ACTION_BITS) || *id < 1u ? 2u : *id
        + 1u);
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
        delta[wheel] = enc_travel_signed(input->count[wheel],
            control->previous[wheel]);
        edges += delta[wheel] < 0 ? (uint64_t)(-(int64_t)delta[wheel])
            : (uint32_t)delta[wheel];
        control->previous[wheel] = input->count[wheel];
    }
    uint32_t maximum = (uint32_t)(2.0f * BYPASS_TRAVEL_MM * ENC_COUNTS_PER_MM);
    if (control->edge_counts > maximum || edges > maximum
        - control->edge_counts)
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
    float middle = ((float)control->heading_cdeg + heading) * 0.5f
        * BYPASS_RAD_PER_CDEG;
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
    if (BYPASS_ROUTE_OFFSET == control->route || BYPASS_ROUTE_OUT
        == control->route)
    {
        return side * 90;
    }
    if (BYPASS_ROUTE_RETURN == control->route || BYPASS_ROUTE_IN
        == control->route)
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

/* Sideways distance from the servo axis to the box's edge on that side. The
   profile gets each edge from the forward range and the edge ray's angle:
   range * tan(angle). */
static float edge_mm(const ObstacleProfile *profile, bool left)
{
    return (left ? profile->obstacle_left_mm : -profile->obstacle_left_mm) +
        profile->width_mm * 0.5f;
}

/* Axle to box edge while passing: half the car's length, or more if that would
   leave too little above the too-close limit to absorb scan and heading error.
   */
static float pass_distance_mm(bool left)
{
    return fmaxf(BYPASS_BODY_HALF_LENGTH_MM,
        facing_half_mm(left) + BYPASS_SAFE_GAP_MM + BYPASS_PASS_ALLOWANCE_MM);
}

/* The leg out, at right angles to the line. */
static float offset_mm(const ObstacleProfile *profile, bool left)
{
    return edge_mm(profile, left) + pass_distance_mm(left);
}

/* A side the scan alone proves clear is taken; otherwise the shorter distance
   to an edge picks the side and a tie goes left. Clearance 0 is unproven, not
   blocked, and two noisy clearances are never compared. */
static bool pass_left(const ObstacleProfile *profile)
{
    bool left_clear = profile->left_clearance_mm >= MISSION_BYPASS_MIN_CLEAR_MM;
    bool right_clear =
        profile->right_clearance_mm >= MISSION_BYPASS_MIN_CLEAR_MM;
    if (left_clear != right_clear)
    {
        return left_clear;
    }
    return edge_mm(profile, true) <= edge_mm(profile, false);
}

static bool side_usable(const ObstacleProfile *profile, bool left)
{
    float offset = offset_mm(profile, left);
    return profile->closest_mm > pivot_reach_mm(left) &&
        offset > 0.0f && offset <= BYPASS_MAX_OFFSET_MM;
}

uint8_t bypass_control_backup_mm(const ObstacleProfile *profile,
    uint16_t planning_width_mm)
{
    (void)planning_width_mm;
    float least = pivot_reach_mm(pass_left(profile)) +
        BYPASS_BACKUP_MARGIN_MM - profile->closest_mm;
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
    return input != NULL && profile != NULL && input->stationary &&
        input->functional && profile->valid &&
        planning_width_mm >= OBSTACLE_EXPECTED_MAX_WIDTH_MM &&
        side_usable(profile, pass_left(profile));
}

bool bypass_control_begin(bypass_control_t *control,
    const bypass_input_t *input, const ObstacleProfile *profile,
    uint16_t planning_width_mm, uint8_t *action_id)
{
    if (NULL == control || NULL == action_id ||
        !bypass_control_can_begin(input, profile, planning_width_mm))
    {
        return false;
    }
    bool left = pass_left(profile);
    float offset = offset_mm(profile, left);
    *control = (bypass_control_t){.origin = {input->count[0], input->count[1]},
        .previous = {input->count[0], input->count[1]},
        .started_ms = input->now_ms,
        .left_side = left,
        .offset_mm = (int16_t)lroundf(left ? offset : -offset),
        .centre_mm = profile->obstacle_left_mm,
        .half_width_mm = (uint16_t)(profile->width_mm / 2u),
        .obstacle_front_mm = profile->obstacle_forward_mm};
    turn(control, input, BYPASS_ROUTE_OUT, left ? 90 : -90, action_id);
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

/* Too close while passing: brake, then step further out and pass again. The
   room the scan measured limits this. Pivoting away swings the rear corner
   toward the box, so if the box is already within that swing, stop instead. */
static void step_out(bypass_control_t *control, float across)
{
    float box_ahead = (float)control->obstacle_front_mm - forward_mm(control);
    int offset = abs(control->offset_mm) + (int)BYPASS_STEP_OUT_MM;
    if (offset > (int)BYPASS_MAX_OFFSET_MM ||
        hypotf(box_ahead, across) <= hypotf(BYPASS_BODY_REAR_MM,
            facing_half_mm(control->left_side)))
    {
        fail(control);
        return;
    }
    control->offset_mm = (int16_t)(control->left_side ? offset : -offset);
    control->close_samples = 0u;
    control->edge_open = 0u;
    control->flank_mm = 0u;
    control->work = BYPASS_WORK_STOP;
}

/* The side ray meets the box's side until past its rear. A reading beyond the
   box's centreline, or none, has to last BYPASS_EDGE_MM of travel to count as
   clear. */
static void side_observation(bypass_control_t *control,
    const bypass_input_t *input)
{
    float side = control->left_side ? 1.0f : -1.0f;
    float angle = SERVO_LOOK_DEG * BYPASS_RAD_PER_DEG;
    float to_centre = side * (left_mm(control) - control->centre_mm);
    float across = input->side_mm * sinf(angle);
    /* Raw range, not the sideways part: the beam is wide, so a range this short
       is close whichever way it points, and a normal pass does not step out. */
    if (input->side_valid &&
        input->side_mm < facing_half_mm(control->left_side)
            + BYPASS_SAFE_GAP_MM)
    {
        if (++control->close_samples >= BYPASS_CLOSE_SAMPLES)
        {
            step_out(control, across);
        }
        return;
    }
    control->close_samples = 0u;
    /* The right-angle leg ends level with the stop, so the ray starts short of
       the box: nothing there yet says nothing about its rear. */
    if (forward_mm(control) + OBSTACLE_SONAR_FORWARD_MM +
        fmaxf(to_centre - control->half_width_mm, 0.0f) / tanf(angle) <
        control->obstacle_front_mm)
    {
        return;
    }
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
    /* A flat side at this angle can return no echo, or only its front corner,
       so
       the ray can leave long before the rear. Trust it only when it reads
       deeper
       than this. This only times the pivot back; depth is not reported. */
    rear = fmaxf(rear, (float)control->obstacle_front_mm + BYPASS_MIN_DEPTH_MM);
    control->rear_clear_q8 = (int32_t)lroundf(
        (rear + facing_half_mm(control->left_side) + BYPASS_REAR_GAP_MM)
            * BYPASS_Q8);
    control->route = BYPASS_ROUTE_REAR;
}

void bypass_control_step(bypass_control_t *control, const bypass_input_t *input,
    uint8_t *action_id, bypass_output_t *output)
{
    *output = (bypass_output_t){.command = motion_command_make(MOTION_MODE_STOP,
        0.0f, 0.0f, 0, 0u, input->now_ms), .route = BYPASS_ROUTE_FAILED};
    if (input->cancel || !input->functional ||
        robot_time_elapsed(input->now_ms, control->started_ms)
            >= MISSION_BYPASS_TIMEOUT_MS ||
        !pose_step(control, input) || control->route > BYPASS_ROUTE_FAILED)
    {
        fail(control);
    }
    if (BYPASS_ROUTE_RETURN == control->route)
    {
        if (!input->line_functional ||
            robot_time_elapsed(input->now_ms, input->line_ms) > IR_MAX_GAP_MS)
        {
            fail(control);
        }
        else if (!input->line_black)
        {
            control->return_white = 1u;
        }
        else if (control->return_white)
        {
            /* The line itself ends the return, so wheel drift over the bypass
               does not matter. Mission crosses it like a junction turn. Black
               under a sensor as the leg starts is not the line: the leg starts
               further out than the sensors reach (2026-11-09). */
            control->route = BYPASS_ROUTE_DONE;
        }
    }
    if (BYPASS_ROUTE_DONE == control->route || BYPASS_ROUTE_FAILED
        == control->route)
    {
        output->route = (bypass_route_t)control->route;
        return;
    }
    int side = control->left_side ? 1 : -1;
    bool finished = input->motion_action == control->action_id &&
        RD_MOTION_DONE == input->motion_status;
    bool action_failed = input->motion_action == control->action_id &&
        RD_MOTION_FAILED == input->motion_status;
    if (BYPASS_WORK_TURN == control->work)
    {
        bool swept_hazard = control->route != BYPASS_ROUTE_PARALLEL &&
            input->front_valid &&
            robot_time_elapsed(input->now_ms, input->front_ms)
                <= BYPASS_FRONT_MAX_AGE_MS &&
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
            else if (BYPASS_ROUTE_OUT == control->route)
            {
                drive(control, input, BYPASS_ROUTE_OFFSET,
                    fabsf((float)control->offset_mm - left_mm(control)),
                        action_id);
            }
            else if (BYPASS_ROUTE_PARALLEL == control->route)
            {
                control->route = BYPASS_ROUTE_PASS;
                control->work = BYPASS_WORK_LOOK;
                control->phase_ms = input->now_ms;
                control->sensor_sequence = input->side_sequence;
            }
            else if (BYPASS_ROUTE_IN == control->route)
            {
                control->return_white = 0u;
                drive(control, input, BYPASS_ROUTE_RETURN,
                    fabsf(left_mm(control)), action_id);
            }
            else
            {
                fail(control);
            }
        }
    }
    else if (BYPASS_WORK_LOOK == control->work)
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
    else if (BYPASS_WORK_STOP == control->work)
    {
        if (input->stationary)
        {
            if (BYPASS_ROUTE_REAR == control->route)
            {
                turn(control, input, BYPASS_ROUTE_IN, -side * 90, action_id);
            }
            else
            {
                turn(control, input, BYPASS_ROUTE_OUT, side * 90, action_id);
            }
        }
    }
    else if (BYPASS_WORK_DRIVE == control->work)
    {
        bool passing = BYPASS_ROUTE_PASS == control->route ||
            BYPASS_ROUTE_REAR == control->route;
        if (action_failed || !heading_held(control) ||
            (!passing && !front_clear(input)) ||
            robot_time_elapsed(input->now_ms, control->action_ms) >=
            MOTION_ACTION_TIMEOUT_MS)
        {
            fail(control);
        }
        else if (passing)
        {
            if (BYPASS_ROUTE_PASS == control->route &&
                input->side_sequence != control->sensor_sequence)
            {
                control->sensor_sequence = input->side_sequence;
                side_observation(control, input);
                if (BYPASS_ROUTE_REAR == control->route)
                {
                    /* Slow to rest at the point that clears the box, or as soon
                       after it as the stop allows. */
                    drive(control, input, BYPASS_ROUTE_REAR, fmaxf(
                        (float)control->rear_clear_q8 / BYPASS_Q8
                            - forward_mm(control),
                        motion_stop_distance_mm(BYPASS_SPEED_MMPS)), action_id);
                    finished = false;
                }
            }
            if (finished && BYPASS_ROUTE_REAR == control->route)
            {
                control->work = BYPASS_WORK_STOP;
            }
            else if (finished && BYPASS_WORK_DRIVE == control->work)
            {
                /* the leg ran out with the box still beside the car */
                fail(control);
            }
        }
        else if (finished)
        {
            if (BYPASS_ROUTE_OFFSET == control->route)
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
    if (BYPASS_ROUTE_FAILED == control->route || BYPASS_ROUTE_DONE
        == control->route)
    {
        output->side_watch = false;
        return;
    }
    /* Start the look during the turn to parallel, so the car barely waits for
       the servo. */
    output->side_watch = BYPASS_ROUTE_PARALLEL == control->route ||
        BYPASS_ROUTE_PASS == control->route ||
        BYPASS_ROUTE_REAR == control->route;
    output->side_angle_deg = (int16_t)(-side * SERVO_LOOK_DEG);
    if (BYPASS_WORK_TURN == control->work)
    {
        output->command = motion_command_make(MOTION_MODE_TURN,
            BYPASS_TURN_SPEED_MMPS,
            control->action_target, 0, control->action_id, input->now_ms);
    }
    else if (BYPASS_WORK_DRIVE == control->work)
    {
        output->command = motion_command_make(MOTION_MODE_DISTANCE,
            BYPASS_SPEED_MMPS,
            control->action_target, BYPASS_ROUTE_REAR == control->route
                ? MOTION_TURN_RAMPED : 0,
            control->action_id, input->now_ms);
    }
}

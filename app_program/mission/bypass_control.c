#include "bypass_control.h"
#include "../common/robot_time_math.h"
#include <math.h>
#include <stdlib.h>

#define BYPASS_RAD_PER_CDEG 0.000174532925f
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

static void sense(bypass_control_t *control, const bypass_input_t *input, bool side)
{
    control->work = side ? BYPASS_WORK_SIDE : BYPASS_WORK_FRONT;
    control->phase_ms = input->now_ms;
    control->sensor_sequence = side ? input->side_sequence : input->front_sequence;
    control->attempts = 0u;
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

static float leg_remaining(const bypass_control_t *control)
{
    float side = control->left_side ? 1.0f : -1.0f;
    switch (control->route)
    {
    case BYPASS_ROUTE_OFFSET:
        return side * ((float)control->offset_mm - left_mm(control));
    case BYPASS_ROUTE_RETURN:
        return side * left_mm(control);
    case BYPASS_ROUTE_REAR:
        return (float)control->rear_clear_q8 / BYPASS_Q8 - forward_mm(control);
    case BYPASS_ROUTE_PASS:
        return BYPASS_PASS_MAX_MM - (forward_mm(control) - control->pass_start_mm);
    default:
        return 0.0f;
    }
}

static int expected_heading(const bypass_control_t *control)
{
    int side = control->left_side ? 1 : -1;
    if (control->route == BYPASS_ROUTE_OFFSET || control->route == BYPASS_ROUTE_OUT)
    {
        return side * 90;
    }
    if (control->route == BYPASS_ROUTE_RETURN || control->route == BYPASS_ROUTE_IN)
    {
        return -side * 90;
    }
    return 0;
}

static float front_usable_mm(const bypass_input_t *input)
{
    return input->front_mm - fmaxf(0.0f,
        BYPASS_BODY_LENGTH_MM * 0.5f - OBSTACLE_SONAR_FORWARD_MM) -
        BYPASS_CLEARANCE_MM - BYPASS_FRONT_RESERVE_MM;
}

static bool front_supported(const bypass_control_t *control, const bypass_input_t *input)
{
    if (!input->front_valid || input->front_sequence == 0u ||
        robot_time_elapsed(input->now_ms, input->front_ms) > BYPASS_FRONT_MAX_AGE_MS)
    {
        return false;
    }
    float moved = enc_mean_travel(input->count, input->front_count);
    float clearance = front_usable_mm(input);
    return moved >= 0.0f && clearance > moved / ENC_COUNTS_PER_MM &&
        fabsf((float)control->heading_cdeg / 100.0f - expected_heading(control)) <=
            BYPASS_RETURN_HEADING_DEG;
}

bool bypass_control_begin(bypass_control_t *control, const bypass_input_t *input,
    const ObstacleProfile *profile, uint16_t planning_width_mm, uint8_t *action_id)
{
    if (control == NULL || input == NULL || profile == NULL || action_id == NULL ||
        !input->stationary || !input->functional || !profile->valid ||
        planning_width_mm < OBSTACLE_EXPECTED_MAX_WIDTH_MM ||
        profile->closest_mm <= hypotf(BYPASS_BODY_LENGTH_MM * 0.5f,
            BYPASS_BODY_WIDTH_MM * 0.5f) + BYPASS_CLEARANCE_MM)
    {
        return false;
    }
    bool left = profile->left_clearance_mm >= profile->right_clearance_mm;
    uint16_t clearance = left ? profile->left_clearance_mm : profile->right_clearance_mm;
    if (clearance < fmaxf(MISSION_BYPASS_MIN_CLEAR_MM,
        BYPASS_BODY_WIDTH_MM + 2.0f * BYPASS_CLEARANCE_MM))
    {
        return false;
    }
    float offset = (left ? profile->obstacle_left_mm : -profile->obstacle_left_mm) +
        planning_width_mm * 0.5f + BYPASS_BODY_WIDTH_MM * 0.5f + BYPASS_CLEARANCE_MM;
    offset = fmaxf(offset, MISSION_BYPASS_OFFSET_MM);
    if (offset > BYPASS_MAX_OFFSET_MM)
    {
        return false;
    }
    *control = (bypass_control_t){.origin = {input->count[0], input->count[1]},
        .previous = {input->count[0], input->count[1]}, .started_ms = input->now_ms,
        .left_side = left, .offset_mm = (int16_t)lroundf(left ? offset : -offset),
        .obstacle_front_mm = profile->obstacle_forward_mm};
    turn(control, input, BYPASS_ROUTE_OUT, left ? 90 : -90, action_id);
    return true;
}

static bool sample_ready(bypass_control_t *control, const bypass_input_t *input, bool side)
{
    uint32_t sequence = side ? input->side_sequence : input->front_sequence;
    uint32_t sampled = side ? input->side_ms : input->front_ms;
    if (!input->stationary || sequence == 0u || sequence == control->sensor_sequence ||
        (int32_t)(sampled - control->phase_ms) <= 0)
    {
        return false;
    }
    control->sensor_sequence = sequence;
    bool valid = side ? input->side_valid : input->front_valid;
    if (!valid || robot_time_elapsed(input->now_ms, sampled) > BYPASS_FRONT_MAX_AGE_MS)
    {
        ++control->attempts;
        if (control->attempts >= BYPASS_SENSE_ATTEMPTS)
        {
            fail(control);
        }
        return false;
    }
    return true;
}

static void side_observation(bypass_control_t *control, const bypass_input_t *input,
    bypass_output_t *output)
{
    float side_clearance = input->side_mm * sinf(SERVO_MAX_DEG * 0.0174532925f);
    if (side_clearance < BYPASS_BODY_WIDTH_MM * 0.5f + BYPASS_CLEARANCE_MM)
    {
        fail(control);
        return;
    }
    if (!control->baseline_valid)
    {
        if (input->side_mm <= BYPASS_SIDE_PRESENT_MM)
        {
            control->baseline_mm = (uint16_t)input->side_mm;
            control->baseline_valid = 1u;
        }
        else if (forward_mm(control) - control->pass_start_mm >= BYPASS_BASELINE_SEARCH_MM)
        {
            fail(control);
            return;
        }
    }
    else if (input->side_mm > (uint32_t)control->baseline_mm + BYPASS_EDGE_ENTER_MM)
    {
        if (control->edge_streak == 0u)
        {
            /* Keep the first edge position while later samples confirm it. */
            control->edge_forward_q8 = control->forward_q8;
        }
        if (control->edge_streak < BYPASS_EDGE_SAMPLES)
        {
            ++control->edge_streak;
        }
        if (control->edge_streak >= BYPASS_EDGE_SAMPLES && control->route != BYPASS_ROUTE_REAR)
        {
            float rear = (float)control->edge_forward_q8 / BYPASS_Q8 +
                OBSTACLE_SONAR_FORWARD_MM + control->baseline_mm *
                cosf(SERVO_MAX_DEG * 0.0174532925f);
            float clear = rear + hypotf(BYPASS_BODY_LENGTH_MM * 0.5f,
                BYPASS_BODY_WIDTH_MM * 0.5f) + BYPASS_CLEARANCE_MM;
            if (clear >= BYPASS_TRAVEL_MM)
            {
                fail(control);
                return;
            }
            control->rear_clear_q8 = (int32_t)lroundf(clear * BYPASS_Q8);
            control->route = BYPASS_ROUTE_REAR;
            float depth = fmaxf(0.0f, rear - control->obstacle_front_mm);
            control->depth_mm = (uint16_t)lroundf(fminf(depth, 65535.0f));
            output->depth_event = true;
            output->depth_mm = control->depth_mm;
        }
    }
    else
    {
        control->edge_streak = 0u;
        if (input->side_mm <= (uint32_t)control->baseline_mm + BYPASS_EDGE_EXIT_MM)
        {
            if (input->side_mm < control->baseline_mm)
            {
                control->baseline_mm = (uint16_t)input->side_mm;
            }
            control->route = BYPASS_ROUTE_PASS;
        }
    }
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
    if (control->route == BYPASS_ROUTE_DONE || control->route == BYPASS_ROUTE_FAILED)
    {
        output->route = (bypass_route_t)control->route;
        return;
    }
    int side = control->left_side ? 1 : -1;
    if (control->work == BYPASS_WORK_TURN)
    {
        bool swept_hazard = input->front_valid &&
            robot_time_elapsed(input->now_ms, input->front_ms) <= BYPASS_FRONT_MAX_AGE_MS &&
            input->front_mm + OBSTACLE_SONAR_FORWARD_MM <=
                hypotf(BYPASS_BODY_LENGTH_MM * 0.5f, BYPASS_BODY_WIDTH_MM * 0.5f) + BYPASS_CLEARANCE_MM;
        if (swept_hazard || robot_time_elapsed(input->now_ms, control->action_ms) >= MOTION_ACTION_TIMEOUT_MS ||
            (input->motion_action == control->action_id && input->motion_status == RD_MOTION_FAILED))
        {
            fail(control);
        }
        else if (input->motion_action == control->action_id && input->motion_status == RD_MOTION_DONE)
        {
            if ((unsigned)abs(control->heading_cdeg - expected_heading(control) * 100) >
                BYPASS_RETURN_HEADING_DEG * 100)
            {
                fail(control);
            }
            else if (control->route == BYPASS_ROUTE_OUT)
            {
                control->route = BYPASS_ROUTE_OFFSET;
                sense(control, input, false);
            }
            else if (control->route == BYPASS_ROUTE_PARALLEL)
            {
                control->route = BYPASS_ROUTE_PASS;
                control->pass_start_mm = (int16_t)lroundf(forward_mm(control));
                sense(control, input, true);
            }
            else if (control->route == BYPASS_ROUTE_IN)
            {
                control->route = BYPASS_ROUTE_RETURN;
                sense(control, input, false);
            }
            else
            {
                control->route = BYPASS_ROUTE_LINE;
                sense(control, input, false);
                control->line_started_ms = input->now_ms;
                control->chunk_origin[0] = input->count[0];
                control->chunk_origin[1] = input->count[1];
                control->line_sampled_ms = input->line_ms;
                control->visible_streak = 0u;
                control->line_seen = 0u;
            }
        }
    }
    else if (control->work == BYPASS_WORK_SIDE)
    {
        output->side_watch = true;
        output->side_angle_deg = (int16_t)(-side * SERVO_MAX_DEG);
        if (sample_ready(control, input, true))
        {
            side_observation(control, input, output);
            if (control->route != BYPASS_ROUTE_FAILED)
            {
                if (control->route == BYPASS_ROUTE_REAR && leg_remaining(control) <= MOTION_DIST_TOL_MM)
                {
                    turn(control, input, BYPASS_ROUTE_IN, -side * 90, action_id);
                }
                else
                {
                    sense(control, input, false);
                }
            }
        }
    }
    else if (control->work == BYPASS_WORK_FRONT)
    {
        if (sample_ready(control, input, false))
        {
            if (!front_supported(control, input))
            {
                fail(control);
            }
            else
            {
                float remaining = leg_remaining(control);
                if (control->route == BYPASS_ROUTE_LINE)
                {
                    control->work = BYPASS_WORK_LINE;
                }
                else if (remaining <= MOTION_DIST_TOL_MM)
                {
                    if (control->route == BYPASS_ROUTE_OFFSET)
                    {
                        turn(control, input, BYPASS_ROUTE_PARALLEL, 0, action_id);
                    }
                    else if (control->route == BYPASS_ROUTE_RETURN)
                    {
                        turn(control, input, BYPASS_ROUTE_FORWARD, 0, action_id);
                    }
                    else
                    {
                        fail(control);
                    }
                }
                else
                {
                    float usable = front_usable_mm(input);
                    float target = fminf(remaining, fminf(BYPASS_CHUNK_MM, usable));
                    if (target <= MOTION_DIST_TOL_MM)
                    {
                        fail(control);
                    }
                    else
                    {
                        control->action_target = (int16_t)floorf(target);
                        control->action_id = next_id(action_id);
                        control->action_ms = input->now_ms;
                        control->work = BYPASS_WORK_DRIVE;
                    }
                }
            }
        }
    }
    else if (control->work == BYPASS_WORK_DRIVE)
    {
        if (!front_supported(control, input) ||
            robot_time_elapsed(input->now_ms, control->action_ms) >= MOTION_ACTION_TIMEOUT_MS ||
            (input->motion_action == control->action_id && input->motion_status == RD_MOTION_FAILED))
        {
            fail(control);
        }
        else if (input->motion_action == control->action_id && input->motion_status == RD_MOTION_DONE)
        {
            sense(control, input, control->route == BYPASS_ROUTE_PASS || control->route == BYPASS_ROUTE_REAR);
        }
    }
    else if (control->work == BYPASS_WORK_LINE)
    {
        float lateral = fabsf(left_mm(control));
        float heading = fabsf((float)control->heading_cdeg / 100.0f);
        float travel = enc_mean_travel(input->count, control->chunk_origin) /
            ENC_COUNTS_PER_MM;
        if (!input->line_functional || !front_supported(control, input) ||
            robot_time_elapsed(input->now_ms, input->line_ms) > IR_MAX_GAP_MS ||
            travel >= BYPASS_REACQUIRE_MM || lateral > BYPASS_RETURN_LATERAL_MM ||
            heading > BYPASS_RETURN_HEADING_DEG)
        {
            fail(control);
        }
        else if (input->line_ms != control->line_sampled_ms)
        {
            control->line_sampled_ms = input->line_ms;
            /* Aligned white/white counts only after the line was crossed. */
            control->line_seen = control->line_seen || input->line_black;
            if (control->line_seen && input->line_visible &&
                abs(input->line_error_q) <= RECOVERY_ALIGN_ERROR_Q)
            {
                if (control->visible_streak < MISSION_CONFIRM_SAMPLES)
                {
                    ++control->visible_streak;
                }
                if (control->visible_streak >= MISSION_CONFIRM_SAMPLES)
                {
                    control->route = BYPASS_ROUTE_DONE;
                }
            }
            else
            {
                control->visible_streak = 0u;
            }
        }
    }
    else
    {
        fail(control);
    }
    if ((control->work == BYPASS_WORK_SIDE || control->work == BYPASS_WORK_FRONT) &&
        robot_time_elapsed(input->now_ms, control->phase_ms) >= BYPASS_SENSE_TIMEOUT_MS)
    {
        fail(control);
    }
    if ((control->route == BYPASS_ROUTE_PASS || control->route == BYPASS_ROUTE_REAR) &&
        forward_mm(control) - control->pass_start_mm >= BYPASS_PASS_MAX_MM)
    {
        fail(control);
    }
    output->route = (bypass_route_t)control->route;
    if (control->route == BYPASS_ROUTE_FAILED || control->route == BYPASS_ROUTE_DONE)
    {
        output->side_watch = false;
        return;
    }
    output->side_watch = control->work == BYPASS_WORK_SIDE;
    output->side_angle_deg = (int16_t)(-side * SERVO_MAX_DEG);
    if (control->work == BYPASS_WORK_TURN)
    {
        output->command = motion_command_make(MOTION_MODE_TURN, BYPASS_TURN_SPEED_MMPS,
            control->action_target, 0, control->action_id, input->now_ms);
    }
    else if (control->work == BYPASS_WORK_DRIVE)
    {
        output->command = motion_command_make(MOTION_MODE_DISTANCE, BYPASS_SPEED_MMPS,
            control->action_target, 0, control->action_id, input->now_ms);
    }
    else if (control->work == BYPASS_WORK_LINE)
    {
        output->command = motion_command_make(MOTION_MODE_FOLLOW, BYPASS_LINE_SPEED_MMPS,
            0.0f, input->line_visible ? input->line_steering_q : 0,
            1u, input->line_ms);
    }
}

#include "recovery_control.h"
#include "../motion/encoder_math.h"
#include "../common/robot_time_math.h"
#include <math.h>

void recovery_control_begin(recovery_control_t *control, const recovery_input_t *input)
{
    *control = (recovery_control_t){.origin = {input->count[0], input->count[1]},
        .started_ms = input->now_ms};
}

static void start_turn(recovery_control_t *control, recovery_output_t *output,
    float degrees)
{
    /* Heading plus the opposite endpoint fits
       Motion's +/-360 degree representation. Round once and hold the goal. */
    control->turn_degrees = (int16_t)lroundf(degrees);
    control->turning = 1u;
    output->new_turn = true;
}

void recovery_control_step(recovery_control_t *control, const recovery_input_t *input,
    recovery_output_t *output)
{
    *output = (recovery_output_t){.phase = RECOVERY_FAILED};
    int32_t left = enc_travel_signed(input->count[0], control->origin[0]);
    int32_t right = enc_travel_signed(input->count[1], control->origin[1]);
    float heading = enc_heading_deg(left, right);
    /* Opposite wheel directions are expected for a pivot. Bound absolute
       wheel excursion; the deadline bounds repeated back-and-forth. */
    float excursion = (fabsf((float)left) + fabsf((float)right)) *
        0.5f / ENC_COUNTS_PER_MM;
    if (input->cancel || input->hazard || !input->functional ||
        robot_time_elapsed(input->now_ms, input->sampled_ms) > IR_MAX_GAP_MS ||
        robot_time_elapsed(input->now_ms, control->started_ms) >= RECOVERY_MS ||
        excursion >= RECOVERY_TRAVEL_MM ||
        fabsf(heading) > RECOVERY_TARGET_DEG + RECOVERY_OVERSHOOT_DEG ||
        control->phase >= RECOVERY_FAILED ||
        (control->turning && input->motion_failed))
    {
        control->phase = RECOVERY_FAILED;
        return;
    }
    output->heading_cdeg = (int16_t)lroundf(heading * 100.0f);
    if (control->phase == RECOVERY_WAIT)
    {
        output->phase = RECOVERY_WAIT;
        if (!input->stationary)
        {
            return;
        }
        /* Measure sweep angles from the stopped pose, excluding coast-down. */
        control->origin[0] = input->count[0];
        control->origin[1] = input->count[1];
        heading = 0.0f;
        control->phase = RECOVERY_SEARCH;
    }
    if (control->phase == RECOVERY_DONE)
    {
        output->phase = RECOVERY_DONE;
        return;
    }
    /* White/white alone is blank floor; it aligns only after a sensor crossed
       the line during the search. */
    control->seen = control->seen || input->black;
    if (input->visible && control->seen)
    {
        control->turning = 0u;
        control->phase = RECOVERY_DONE;
    }
    else
    {
        float target = !control->right ?
            RECOVERY_TARGET_DEG : -RECOVERY_TARGET_DEG;
        bool reached = target > 0.0f ?
            heading >= target - RECOVERY_TARGET_TOL_DEG :
            heading <= target + RECOVERY_TARGET_TOL_DEG;
        if (reached)
        {
            control->turning = 0u;
            control->right = !control->right;
            target = -target;
        }
        else if (control->turning && input->motion_done)
        {
            /* Motion completed outside the expected heading tolerance. */
            control->phase = RECOVERY_FAILED;
            return;
        }
        if (!control->turning && input->stationary)
        {
            start_turn(control, output, target - heading);
        }
    }
    output->phase = (recovery_phase_t)control->phase;
    output->turn = control->turning;
    output->turn_degrees = control->turn_degrees;
}

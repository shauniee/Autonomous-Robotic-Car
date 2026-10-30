#include "recovery_control.h"
#include "../subsystem2/encoder_math.h"
#include "../common/robot_time_math.h"
#include <math.h>
#include <stdlib.h>

void recovery_control_begin(recovery_control_t *control, const recovery_input_t *input)
{
    *control = (recovery_control_t){.origin = {input->count[0], input->count[1]},
        .started_ms = input->now_ms, .sampled_ms = input->sampled_ms};
}

void recovery_control_step(recovery_control_t *control, const recovery_input_t *input,
    recovery_output_t *output)
{
    *output = (recovery_output_t){.phase = RECOVERY_FAILED};
    int32_t left = enc_travel_signed(input->count[0], control->origin[0]);
    int32_t right = enc_travel_signed(input->count[1], control->origin[1]);
    float heading = ((float)right - (float)left) / ENC_COUNTS_PER_MM /
        TRACK_WIDTH_MM * 57.2957795f;
    float travel = ((float)left + (float)right) * 0.5f / ENC_COUNTS_PER_MM;
    if (input->cancel || input->hazard || !input->functional ||
        robot_time_elapsed(input->now_ms, input->sampled_ms) > IR_MAX_GAP_MS ||
        robot_time_elapsed(input->now_ms, control->started_ms) >= RECOVERY_MS ||
        left < 0 || right < 0 || travel >= RECOVERY_TRAVEL_MM ||
        fabsf(heading) > RECOVERY_TARGET_DEG || control->leg >= 4u ||
        control->phase == RECOVERY_FAILED)
    {
        control->phase = RECOVERY_FAILED;
        return;
    }
    output->heading_cdeg = (int16_t)lroundf(heading * 100.0f);
    if (control->phase == RECOVERY_DONE)
    {
        output->phase = RECOVERY_DONE;
        return;
    }
    if (input->visible)
    {
        control->phase = RECOVERY_ALIGN;
        if (input->sampled_ms != control->sampled_ms)
        {
            control->sampled_ms = input->sampled_ms;
            if (abs(input->error_q) <= RECOVERY_ALIGN_ERROR_Q &&
                fabsf(heading) <= RECOVERY_ALIGN_HEADING_DEG)
            {
                if (control->stable < MISSION_CONFIRM_SAMPLES)
                {
                    ++control->stable;
                }
            }
            else
            {
                control->stable = 0u;
            }
        }
        if (control->stable >= MISSION_CONFIRM_SAMPLES)
        {
            control->phase = RECOVERY_DONE;
            output->phase = RECOVERY_DONE;
            return;
        }
        output->phase = RECOVERY_ALIGN;
        output->steering_q = fabsf(heading) > RECOVERY_ALIGN_HEADING_DEG ?
            (heading > 0.0f ? RECOVERY_STEER_Q : -RECOVERY_STEER_Q) : input->steering_q;
        return;
    }
    control->phase = RECOVERY_SEARCH;
    control->stable = 0u;
    control->sampled_ms = input->sampled_ms;
    float target = (control->leg & 1u) == 0u ? RECOVERY_TARGET_DEG : -RECOVERY_TARGET_DEG;
    bool reached = target > 0.0f ? heading >= target - RECOVERY_TARGET_TOL_DEG :
        heading <= target + RECOVERY_TARGET_TOL_DEG;
    if (reached)
    {
        ++control->leg;
        if (control->leg >= 4u)
        {
            control->phase = RECOVERY_FAILED;
            return;
        }
        target = (control->leg & 1u) == 0u ? RECOVERY_TARGET_DEG : -RECOVERY_TARGET_DEG;
    }
    output->phase = RECOVERY_SEARCH;
    /* Positive wheel steering turns right; positive encoder heading is left. */
    output->steering_q = target > heading ? -RECOVERY_STEER_Q : RECOVERY_STEER_Q;
}

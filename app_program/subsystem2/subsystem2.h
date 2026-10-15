#ifndef SUBSYSTEM2_H
#define SUBSYSTEM2_H
#include "../app.h"
#include "motion_policy.h"
#include "motion_control.h"
#include "drv_encoder.h"
#define S2_PRI_MOTION 13
typedef struct
{
    uint32_t serviced_ms, sequence, run_epoch;
    bool stopped, brake_acknowledged;
} MotionHealth;
typedef struct
{
    uint32_t sampled_ms, sequence, run_epoch, action_id;
    EncoderCapture encoder;
    float left_m, right_m, left_mps, right_mps, x_m, y_m, yaw_deg;
    float left_duty, right_duty;
    MotionStatus status;
    MotionReason reason;
    bool valid, stationary;
} MotionFeedback;
ER subsystem2_init(void);
ER subsystem2_start(void);
ER s2_command(const MotionCommand *command);
ER s2_feedback_copy(MotionFeedback *feedback);
ER s2_ack(uint32_t action_id);
/* Core-1 runtime callers only; independent persistent stop plus wake-up. */
ER s2_stop(void);
ER s2_reset_stop(void);
ER s2_health(MotionHealth *health);
/* Retired duty producer: returns E_NOSPT, never authorizes open-loop motion. */
ER s2_publish(MotionRequest request);
#endif

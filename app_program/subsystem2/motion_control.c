#include "motion_control.h"
#include "motion_policy.h"
#include "../common/robot_time.h"
#include <math.h>
#include <string.h>

static float limit(float v, float bound)
{
    return isfinite(v) ? fmaxf(-bound, fminf(v, bound)) : NAN;
}
void motion_control_init(MotionController *c, uint32_t epoch)
{
    memset(c, 0, sizeof(*c));
    c->epoch = epoch;
}
bool motion_command_valid(const MotionCommand *r, uint32_t now, uint32_t epoch,
                          const RobotConfig *c)
{
    if (r == NULL || c == NULL || r->run_epoch != epoch || r->action_id == 0u ||
        (unsigned)r->mode > MOTION_DUTY_BENCH ||
        (uint32_t)(now - r->sampled_ms) > 60u ||
        (uint32_t)(now - r->issued_ms) > 60u ||
        (uint32_t)(r->expires_ms - r->issued_ms) > 60u ||
        robot_due(now, r->expires_ms) || !isfinite(r->left_mps) ||
        !isfinite(r->right_mps) || !isfinite(r->target) ||
        !isfinite(r->speed_mps))
    {
        return false;
    }
    if (r->mode == MOTION_BRAKE)
    {
        return true;
    }
    if (r->mode == MOTION_DUTY_BENCH)
    {
        return c->electrical_confirmed &&
               c->commissioning == COMMISSIONING_DUTY &&
               isfinite(r->left_duty) && isfinite(r->right_duty) &&
               fabsf(r->left_duty) <= MOTION_MAX_DUTY &&
               fabsf(r->right_duty) <= MOTION_MAX_DUTY &&
               !robot_due(now, r->deadline_ms) && r->deadline_ms - now <= 2000u;
    }
    if (c->speed_max_mps <= 0.0f || fabsf(r->left_mps) > c->speed_max_mps ||
        fabsf(r->right_mps) > c->speed_max_mps)
    {
        return false;
    }
    if (r->mode == MOTION_SPEED)
    {
        return true;
    }
    return r->speed_mps > 0.0f && r->speed_mps <= c->speed_max_mps &&
           !robot_due(now, r->deadline_ms) &&
           (uint32_t)(r->deadline_ms - now) <= c->action_ms &&
           ((r->mode == MOTION_DISTANCE && fabsf(r->target) <= 5.0f) ||
            (r->mode == MOTION_TURN && fabsf(r->target) <= 360.0f));
}
bool motion_control_accept(MotionController *c, const MotionCommand *r,
                           float left_m, float right_m, uint32_t now,
                           const RobotConfig *config)
{
    if (!isfinite(left_m) || !isfinite(right_m) ||
        !motion_command_valid(r, now, c->epoch, config))
    {
        return false;
    }
    if (r->action_id == c->command.action_id)
    {
        /* Renew a lease without resetting target, origin or deadline. */
        if (r->mode != c->command.mode || r->target != c->command.target ||
            r->deadline_ms != c->command.deadline_ms ||
            r->speed_mps != c->command.speed_mps ||
            r->left_duty != c->command.left_duty ||
            r->right_duty != c->command.right_duty)
        {
            return false;
        }
        c->command = *r;
        return true;
    }
    if ((int32_t)(r->action_id - c->last_id) <= 0 || c->status == MOTION_DONE ||
        c->status == MOTION_FAILED ||
        (c->status == MOTION_ACTIVE && c->command.mode != MOTION_SPEED &&
         r->mode != MOTION_BRAKE))
    {
        return false;
    }
    c->command = *r;
    c->last_id = r->action_id;
    c->origin_left = left_m;
    c->origin_right = right_m;
    c->status = r->mode == MOTION_BRAKE ? MOTION_IDLE : MOTION_ACTIVE;
    c->reason = MOTION_OK;
    c->stalled_left_ms = c->stalled_right_ms = now;
    c->left_pid = c->right_pid = (WheelPid){0};
    return true;
}
static float wheel(WheelPid *p, float target, float speed, float dt,
                   const RobotConfig *c)
{
    if (fabsf(target) < 0.00001f)
    {
        *p = (WheelPid){.previous_speed = speed};
        return 0.0f;
    }
    float error = target - speed;
    float integral = p->integral + error * dt;
    float output = c->kp * error + c->ki * integral -
                   c->kd * (speed - p->previous_speed) / dt;
    if (fabsf(output) <= MOTION_MAX_DUTY || output * error < 0.0f)
    {
        p->integral = integral; /* Conditional anti-windup. */
    }
    p->previous_speed = speed;
    return limit(output, MOTION_MAX_DUTY);
}
void motion_control_step(MotionController *c, float left_m, float right_m,
                         float left_mps, float right_mps, float dt,
                         uint32_t now, const RobotConfig *config, float *left,
                         float *right)
{
    *left = *right = 0.0f;
    if (c->status != MOTION_ACTIVE)
    {
        return;
    }
    bool commissioning = c->command.mode == MOTION_DUTY_BENCH ||
                         (c->command.mode == MOTION_SPEED &&
                          config->commissioning == COMMISSIONING_SPEED &&
                          c->command.deadline_ms != 0u);
    if (commissioning && robot_due(now, c->command.deadline_ms))
    {
        c->status = MOTION_DONE;
        return;
    }
    if (robot_due(now, c->command.expires_ms) ||
        (uint32_t)(now - c->command.sampled_ms) > 60u ||
        ((c->command.mode == MOTION_DISTANCE ||
          c->command.mode == MOTION_TURN ||
          c->command.mode == MOTION_DUTY_BENCH) &&
         robot_due(now, c->command.deadline_ms)))
    {
        c->status = MOTION_FAILED;
        c->reason = MOTION_EXPIRED;
        return;
    }
    if (c->command.mode == MOTION_DUTY_BENCH)
    {
        *left = c->command.left_duty;
        *right = c->command.right_duty;
        return;
    }
    if (!isfinite(dt) || dt <= 0.0f || dt > 0.060f || !isfinite(left_mps) ||
        !isfinite(right_mps) || !isfinite(left_m) || !isfinite(right_m))
    {
        c->status = MOTION_FAILED;
        c->reason = MOTION_ENCODER_FAULT;
        return;
    }
    float l = c->command.left_mps, r = c->command.right_mps;
    if (c->command.mode != MOTION_SPEED)
    {
        float target_l = c->command.target, target_r = target_l;
        float tolerance = config->distance_tolerance_m;
        if (c->command.mode == MOTION_TURN)
        {
            target_r =
                config->track_m * c->command.target * 0.0174532925f * 0.5f;
            target_l = -target_r; /* Positive target turns left. */
            tolerance = config->track_m * config->angle_tolerance_deg *
                        0.0174532925f * 0.5f;
        }
        float el = target_l - (left_m - c->origin_left);
        float er = target_r - (right_m - c->origin_right);
        if (fabsf(el) <= tolerance && fabsf(er) <= tolerance)
        {
            c->status = MOTION_DONE;
            return;
        }
        l = fabsf(el) <= tolerance
                ? 0.0f
                : copysignf(fminf(c->command.speed_mps, fabsf(el) * 2.0f), el);
        r = fabsf(er) <= tolerance
                ? 0.0f
                : copysignf(fminf(c->command.speed_mps, fabsf(er) * 2.0f), er);
    }
    if (fabsf(l) < config->stationary_mps ||
        fabsf(left_mps) >= config->stationary_mps)
    {
        c->stalled_left_ms = now;
    }
    if (fabsf(r) < config->stationary_mps ||
        fabsf(right_mps) >= config->stationary_mps)
    {
        c->stalled_right_ms = now;
    }
    if ((uint32_t)(now - c->stalled_left_ms) >= config->stall_ms ||
        (uint32_t)(now - c->stalled_right_ms) >= config->stall_ms)
    {
        c->status = MOTION_FAILED;
        c->reason = MOTION_STALLED;
        return;
    }
    *left = wheel(&c->left_pid, l, left_mps, dt, config);
    *right = wheel(&c->right_pid, r, right_mps, dt, config);
    if (!isfinite(*left) || !isfinite(*right))
    {
        *left = *right = 0.0f;
        c->status = MOTION_FAILED;
        c->reason = MOTION_BAD_COMMAND;
    }
}
bool motion_control_ack(MotionController *c, uint32_t id)
{
    if (id != c->command.action_id ||
        (c->status != MOTION_DONE && c->status != MOTION_FAILED))
    {
        return false;
    }
    c->status = MOTION_IDLE;
    return true;
}

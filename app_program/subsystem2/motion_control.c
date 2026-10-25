#include "motion_control.h"
#include <math.h>
#include "../common/robot_time_math.h"

#define PI_F 3.14159265f
#define COUNTS_PER_MM (ENC_COUNTS_PER_REV / (PI_F * WHEEL_DIAMETER_MM))
#define LEFT 0
#define RIGHT 1

void motion_control_reset(MotionController *c)
{
    *c = (MotionController){0};
}

static bool command_valid(const MotionCommand *cmd)
{
    if ((unsigned)cmd->mode >= MOTION_MODE_COUNT || cmd->action_id > rd_mask(RD_MOTION_ACTION_BITS) ||
        !isfinite(cmd->speed_mmps) || !isfinite(cmd->steering) || !isfinite(cmd->target) ||
        cmd->speed_mmps < 0.0f || cmd->speed_mmps > MOTION_SPEED_MAX_MMPS) {
        return false;
    }
    switch (cmd->mode) {
    case MOTION_MODE_FOLLOW:
        return cmd->steering >= -1.0f && cmd->steering <= 1.0f;
    case MOTION_MODE_DISTANCE:
        return cmd->speed_mmps > 0.0f;
    case MOTION_MODE_TURN:
        return cmd->speed_mmps > 0.0f && fabsf(cmd->target) <= 360.0f;
    default:
        return true;
    }
}

bool motion_command_fresh(const MotionCommand *cmd, uint32_t now_ms)
{
    return cmd != 0 && command_valid(cmd) &&
           robot_time_elapsed(now_ms, cmd->sampled_ms) <= MOTION_EXPIRY_MS;
}

static float clampf(float v, float bound)
{
    return v > bound ? bound : (v < -bound ? -bound : v);
}

/* PID on counts/s with conditional anti-windup. dt in seconds. */
static float wheel_duty(WheelPid *p, float target_cps, float speed_cps, float dt)
{
    if (fabsf(target_cps) < 0.001f) {
        *p = (WheelPid){.previous_speed = speed_cps};
        return 0.0f;
    }
    float error = target_cps - speed_cps;
    float integral = p->integral + error * dt;
    float output = MOTION_KP * error + MOTION_KI * integral - MOTION_KD * (speed_cps - p->previous_speed) / dt;
    if (fabsf(output) <= MOTION_MAX_DUTY || output * error < 0.0f) {
        p->integral = integral;
    }
    p->previous_speed = speed_cps;
    return clampf(output, MOTION_MAX_DUTY);
}

static void brake(MotionOutput *out, rd_motion_status_t status, uint8_t id)
{
    out->duty[LEFT] = out->duty[RIGHT] = 0.0f;
    out->status = status;
    out->action_id = id;
}

void motion_control_step(MotionController *c, const MotionCommand *cmd,
                         const MotionInput *in, MotionOutput *out)
{
    out->stall = false;
    if (cmd == 0 || cmd->mode == MOTION_MODE_STOP || !motion_command_fresh(cmd, in->now_ms)) {
        /* An action cut short by a stale or missing sample has failed. */
        bool was_running = c->active && c->status == RD_MOTION_RUNNING &&
                           (c->mode == MOTION_MODE_DISTANCE || c->mode == MOTION_MODE_TURN);
        uint8_t id = c->action_id;
        motion_control_reset(c);
        brake(out, was_running ? RD_MOTION_FAILED : RD_MOTION_IDLE, id);
        return;
    }

    if (!c->active || cmd->action_id != c->action_id) {
        *c = (MotionController){0};
        c->active = true;
        c->action_id = cmd->action_id;
        c->mode = cmd->mode;
        c->status = RD_MOTION_RUNNING;
        c->start_ms = c->previous_ms = in->now_ms;
        for (int w = 0; w < 2; ++w) {
            c->origin[w] = c->previous_count[w] = in->count[w];
            c->stalled_since_ms[w] = in->now_ms;
        }
    }

    /* A finished action holds its result until a new action ID replaces it. */
    if (c->status == RD_MOTION_DONE || c->status == RD_MOTION_FAILED) {
        for (int w = 0; w < 2; ++w) {
            c->previous_count[w] = in->count[w];
        }
        c->previous_ms = in->now_ms;
        brake(out, c->status, c->action_id);
        return;
    }

    uint32_t dt_ms = robot_time_elapsed(in->now_ms, c->previous_ms);
    float speed_cps[2] = {0.0f, 0.0f};
    if (dt_ms > 0u) {
        float dt = (float)dt_ms / 1000.0f;
        for (int w = 0; w < 2; ++w) {
            speed_cps[w] = (float)enc_travel_signed(in->count[w], c->previous_count[w]) / dt;
        }
    }
    float dt = dt_ms > 0u ? (float)dt_ms / 1000.0f : (float)MOTION_PERIOD_MS / 1000.0f;
    c->previous_ms = in->now_ms;
    for (int w = 0; w < 2; ++w) {
        c->previous_count[w] = in->count[w];
    }

    float target_cps[2];
    float vmax_cps = cmd->speed_mmps * COUNTS_PER_MM;
    if (cmd->mode == MOTION_MODE_FOLLOW) {
        target_cps[LEFT] = vmax_cps * (1.0f + cmd->steering);
        target_cps[RIGHT] = vmax_cps * (1.0f - cmd->steering);
    } else {
        float goal[2], tol_counts;
        if (cmd->mode == MOTION_MODE_DISTANCE) {
            goal[LEFT] = goal[RIGHT] = cmd->target * COUNTS_PER_MM;
            tol_counts = MOTION_DIST_TOL_MM * COUNTS_PER_MM;
        } else {
            /* Pivot: each wheel travels the arc of half the track; + is left. */
            float arc = TRACK_WIDTH_MM * 0.5f * cmd->target * (PI_F / 180.0f) * COUNTS_PER_MM;
            goal[LEFT] = -arc;
            goal[RIGHT] = arc;
            tol_counts = TRACK_WIDTH_MM * 0.5f * MOTION_ANGLE_TOL_DEG * (PI_F / 180.0f) * COUNTS_PER_MM;
        }
        bool within = true;
        for (int w = 0; w < 2; ++w) {
            float remaining = goal[w] - (float)enc_travel_signed(in->count[w], c->origin[w]);
            if (fabsf(remaining) <= tol_counts) {
                target_cps[w] = 0.0f;
            } else {
                within = false;
                target_cps[w] = copysignf(fminf(vmax_cps, fabsf(remaining) * MOTION_APPROACH_GAIN), remaining);
            }
        }
        if (within) {
            c->status = RD_MOTION_DONE;
            brake(out, RD_MOTION_DONE, c->action_id);
            return;
        }
        if (robot_time_elapsed(in->now_ms, c->start_ms) >= MOTION_ACTION_TIMEOUT_MS) {
            c->status = RD_MOTION_FAILED;
            brake(out, RD_MOTION_FAILED, c->action_id);
            return;
        }
    }

    /* Stall: a wheel told to move but not moving for MOTION_STALL_MS. */
    for (int w = 0; w < 2; ++w) {
        if (fabsf(target_cps[w]) < MOTION_STATIONARY_CPS || fabsf(speed_cps[w]) >= MOTION_STATIONARY_CPS) {
            c->stalled_since_ms[w] = in->now_ms;
        } else if (robot_time_elapsed(in->now_ms, c->stalled_since_ms[w]) >= MOTION_STALL_MS) {
            c->status = RD_MOTION_FAILED;
            brake(out, RD_MOTION_FAILED, c->action_id);
            out->stall = true;
            return;
        }
    }

    for (int w = 0; w < 2; ++w) {
        out->duty[w] = wheel_duty(&c->pid[w], target_cps[w], speed_cps[w], dt);
    }
    out->status = RD_MOTION_RUNNING;
    out->action_id = c->action_id;
}

#include "motion_control.h"
#include <math.h>
#include "../common/robot_time_math.h"

#define PI_F 3.14159265f
#define COUNTS_PER_MM ENC_COUNTS_PER_MM
#define LEFT 0
#define RIGHT 1

void motion_control_reset(MotionController *c)
{
    *c = (MotionController){0};
}

enum { MOTION_SPEED_MASK = 0x1FFu, MOTION_ACTION_MASK = 0x3Fu,
       MOTION_MODE_MASK = 0x3u, MOTION_Q_MASK = 0xFFu };

motion_mode_t motion_cmd_mode(const MotionCommand *c)
{
    return (motion_mode_t)(c->mode_steering & MOTION_MODE_MASK);
}

uint16_t motion_cmd_speed(const MotionCommand *c)
{
    return (uint16_t)(c->speed_action & MOTION_SPEED_MASK);
}

int16_t motion_cmd_target(const MotionCommand *c) { return c->target; }

int8_t motion_cmd_steering(const MotionCommand *c)
{
    unsigned v = (c->mode_steering >> 2u) & MOTION_Q_MASK;
    return (int8_t)(v < 128u ? (int)v : (int)v - 256);
}

uint8_t motion_cmd_action(const MotionCommand *c)
{
    return (uint8_t)((c->speed_action >> 9u) & MOTION_ACTION_MASK);
}

bool motion_command_set(MotionCommand *dst, motion_mode_t mode, float speed_mmps,
                        float target, int steering_q, unsigned action_id, uint32_t sampled_ms)
{
    if (dst == 0) return false;
    *dst = (MotionCommand){0}; /* invalid requests fail closed */
    if ((unsigned)mode >= MOTION_MODE_COUNT || action_id > rd_mask(RD_MOTION_ACTION_BITS) ||
        !isfinite(speed_mmps) || !isfinite(target) ||
        speed_mmps != floorf(speed_mmps) || target != truncf(target) ||
        speed_mmps < 0.0f || speed_mmps > MOTION_SPEED_MAX_MMPS ||
        steering_q < -127 || steering_q > 127 ||
        target < -MOTION_TARGET_MAX_MM || target > MOTION_TARGET_MAX_MM ||
        (mode == MOTION_MODE_TURN && fabsf(target) > 360.0f) ||
        (mode == MOTION_MODE_DISTANCE && speed_mmps == 0.0f) ||
        (mode == MOTION_MODE_TURN && speed_mmps == 0.0f)) return false;
    *dst = (MotionCommand){
        .sampled_ms = sampled_ms,
        .target = (int16_t)target,
        .speed_action = (uint16_t)(((uint16_t)action_id << 9u) | (uint16_t)speed_mmps),
        .mode_steering = (uint16_t)((((uint16_t)(uint8_t)steering_q) << 2u) | (unsigned)mode)
    };
    return true;
}

MotionCommand motion_command_make(motion_mode_t mode, float speed_mmps, float target,
                                  int steering_q, unsigned action_id, uint32_t sampled_ms)
{
    MotionCommand out = {0};
    (void)motion_command_set(&out, mode, speed_mmps, target, steering_q, action_id, sampled_ms);
    return out;
}

static bool command_valid(const MotionCommand *cmd)
{
    if ((cmd->speed_action & 0x8000u) != 0u ||
        (cmd->mode_steering & 0xFC00u) != 0u ||
        motion_cmd_action(cmd) > rd_mask(RD_MOTION_ACTION_BITS) ||
        motion_cmd_speed(cmd) > MOTION_SPEED_MAX_MMPS ||
        motion_cmd_steering(cmd) < -127 ||
        ((motion_cmd_mode(cmd) == MOTION_MODE_DISTANCE || motion_cmd_mode(cmd) == MOTION_MODE_TURN) &&
         motion_cmd_speed(cmd) == 0u))
        return false;

    if (motion_cmd_target(cmd) < -MOTION_TARGET_MAX_MM || motion_cmd_target(cmd) > MOTION_TARGET_MAX_MM)
        return false;
    return motion_cmd_mode(cmd) != MOTION_MODE_TURN ||
           (motion_cmd_target(cmd) >= -360 && motion_cmd_target(cmd) <= 360);
}

bool motion_command_fresh(const MotionCommand *cmd, uint32_t now_ms)
{
    return cmd != 0 && command_valid(cmd) &&
           robot_time_elapsed(now_ms, cmd->sampled_ms) <= MOTION_EXPIRY_MS;
}

/* Feedforward (static friction + speed) plus PID on counts/s, conditional
   anti-windup. dt in seconds. */
static float wheel_duty(WheelPid *p, float target_cps, float speed_cps, float dt)
{
    if (fabsf(target_cps) < 0.001f) {
        *p = (WheelPid){.previous_speed = speed_cps};
        return 0.0f;
    }
    float error = target_cps - speed_cps;
    float integral = p->integral + error * dt;
    float friction = target_cps > 0.0f ? MOTION_KS : -MOTION_KS;	/* duty to start turning at all */
    float output = friction + MOTION_KF * target_cps + MOTION_KP * error + MOTION_KI * integral -
                   MOTION_KD * (speed_cps - p->previous_speed) / dt;
    /* Speed overshoot brakes instead of reversing against the requested
       wheel direction. Position control can still request the opposite
       direction after passing its target; pivots retain signed wheel targets.
       Include both zero clamps in anti-windup. */
    float lower = target_cps > 0.0f ? 0.0f : -MOTION_MAX_DUTY;
    float upper = target_cps > 0.0f ? MOTION_MAX_DUTY : 0.0f;
    float bounded = fminf(fmaxf(output, lower), upper);
    if (output == bounded || (output - bounded) * error < 0.0f)
    {
        p->integral = integral;
    }
    p->previous_speed = speed_cps;
    return bounded;
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
    if (cmd == 0 || motion_cmd_mode(cmd) == MOTION_MODE_STOP || !motion_command_fresh(cmd, in->now_ms)) {
        /* An action cut short by a stale or missing sample has failed. */
        bool was_running = c->active && c->status == RD_MOTION_RUNNING &&
                           (c->mode == MOTION_MODE_DISTANCE || c->mode == MOTION_MODE_TURN);
        uint8_t id = c->action_id;
        motion_control_reset(c);
        brake(out, was_running ? RD_MOTION_FAILED : RD_MOTION_IDLE, id);
        return;
    }

    if (!c->active || motion_cmd_action(cmd) != c->action_id) {
        *c = (MotionController){0};
        c->active = true;
        c->action_id = motion_cmd_action(cmd);
        c->mode = motion_cmd_mode(cmd);
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
    float vmax_cps = motion_cmd_speed(cmd) * COUNTS_PER_MM;
    if (motion_cmd_mode(cmd) == MOTION_MODE_FOLLOW) {
        target_cps[LEFT] = vmax_cps * (1.0f + ((float)motion_cmd_steering(cmd) / 127.0f));
        target_cps[RIGHT] = vmax_cps * (1.0f - ((float)motion_cmd_steering(cmd) / 127.0f));
    } else {
        float goal[2], tol_counts;
        if (motion_cmd_mode(cmd) == MOTION_MODE_DISTANCE) {
            goal[LEFT] = goal[RIGHT] = motion_cmd_target(cmd) * COUNTS_PER_MM;
            tol_counts = MOTION_DIST_TOL_MM * COUNTS_PER_MM;
        } else {
            /* Pivot: each wheel travels the arc of half the track; + is left. */
            float arc = TRACK_WIDTH_MM * 0.5f * motion_cmd_target(cmd) * (PI_F / 180.0f) * COUNTS_PER_MM;
            goal[LEFT] = -arc;
            goal[RIGHT] = arc;
            tol_counts = TRACK_WIDTH_MM * 0.5f * MOTION_ANGLE_TOL_DEG * (PI_F / 180.0f) * COUNTS_PER_MM;
        }
        bool within = true;
        for (int w = 0; w < 2; ++w) {
            float remaining = goal[w] - (float)enc_travel_signed(in->count[w], c->origin[w]);
            if (fabsf(remaining) <= tol_counts)
            {
                target_cps[w] = 0.0f;
                if (motion_cmd_mode(cmd) == MOTION_MODE_TURN)
                {
                    c->turn_arrived |= (uint8_t)(1u << w);
                }
            }
            else
            {
                /* Keep an arrived turn wheel braked while its partner
                   finishes. Re-energizing after drift produces endpoint
                   hunting. Actual position must still qualify for DONE;
                   persistent drift fails at the existing action timeout. */
                within = false;
                if (motion_cmd_mode(cmd) == MOTION_MODE_TURN &&
                    (c->turn_arrived & (1u << w)) != 0u)
                {
                    target_cps[w] = 0.0f;
                }
                else
                {
                    target_cps[w] = copysignf(fminf(vmax_cps,
                        fabsf(remaining) * MOTION_APPROACH_GAIN), remaining);
                }
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

    /* Stall: a wheel told to move but not moving for MOTION_STALL_MS.
       The bench build (NOGUARDS=1) skips it so a slow or noisy encoder never
       brakes the run. */
    for (int w = 0; w < 2 && MOTION_STALL_GUARD_ENABLED && !APP_NO_GUARDS; ++w) {
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

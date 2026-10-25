#include "line_follow.h"
#include "subsystem3.h"
#include "../common/robot_time.h"
#include "../subsystem2/subsystem2.h"
#include "../common/button.h"
#include "../common/drv_buttons.h"

#define FOLLOW_PERIOD_MS 10u
#define FOLLOW_PRIORITY 14
#define RECOVERY_MS 500u
#define BASE_DUTY 0.25f
#define BROAD_DUTY 0.20f
/* Steering magnitude that keeps both wheels within MOTION_MAX_DUTY. */
#define MAX_STEER 0.40f

_Static_assert(BASE_DUTY * (1.0f + MAX_STEER) <= MOTION_MAX_DUTY,
    "line-follow wheel duty must stay within MOTION_MAX_DUTY");
_Static_assert(BROAD_DUTY <= MOTION_MAX_DUTY,
    "broad-black duty must stay within MOTION_MAX_DUTY");

typedef enum
{
    CALIBRATING, READY, FOLLOW_LINE, RECOVER_LINE, STOPPED
} FollowState;

static void transition(FollowState *state, FollowState next)
{
    static const char *const names[] = {
        "CALIBRATING", "READY", "FOLLOW_LINE", "RECOVER_LINE", "STOPPED"
    };
    if (*state != next)
    {
        *state = next;
        tm_printf((UB *)"[LINE] state=%s%s\n", names[next],
            APP_TEST ? " MOTOR_INHIBITED" : "");
    }
}

static void stop(FollowState *state)
{
    /* Do not repeatedly invalidate an acknowledgement while already stopped. */
    if (*state != STOPPED)
    {
        ER er = s2_stop();
        transition(state, STOPPED);
        if (er < E_OK)
        {
            tm_printf((UB *)"[LINE] stop request failed=%d\n", er);
        }
    }
}

static void follow_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    FollowState state = CALIBRATING;
    uint32_t now;
    now = robot_time_ms();
    uint32_t started = now;
    uint32_t next = now;
    uint32_t recovery_started = now;
    uint32_t previous_sequence = 0u;
    uint32_t previous_sample = 0u;
    unsigned visible_streak = 0u;
    int direction = 0;
    Button start_button;
    button_init(&start_button, now);
    tm_printf((UB *)"[LINE] state=CALIBRATING%s\n",
        APP_TEST ? " MOTOR_INHIBITED" : "");
    for (;;)
    {
        now = robot_time_ms();
        ButtonInputs inputs = drv_buttons_read();
        if (inputs.gp21_down)
        {
            stop(&state);
            button_disarm(&start_button);
        }
        ButtonEvent event = button_update(&start_button,
            inputs.gp20_down, now);
        S3IrSnapshot snapshot = {0};
        MotionHealth health = {0};
        bool copied = s3_ir_copy(&snapshot);
        bool healthy = s2_health(&health) >= E_OK;
        now = robot_time_ms();
        ir_snapshot_refresh(&snapshot, now);
        healthy = healthy && ((health.sequence != 0u &&
            (uint32_t)(now - health.serviced_ms) <= IR_MAX_GAP_MS) ||
            (health.sequence == 0u && state == CALIBRATING &&
             (uint32_t)(now - started) <= IR_MAX_GAP_MS));
        if (!copied || !healthy)
        {
            stop(&state);
        }
        else if (!inputs.gp21_down)
        {
            if (state == CALIBRATING && snapshot.calibration_valid)
            {
                transition(&state, READY);
                button_init(&start_button, now);
                event = BUTTON_EVENT_NONE;
            }
            if (state == STOPPED && event == BUTTON_EVENT_PRESSED &&
                health.brake_acknowledged)
            {
                /* Reset is separate from start; keep S2 latched until start. */
                transition(&state, snapshot.calibration_valid ?
                    READY : CALIBRATING);
                event = BUTTON_EVENT_NONE;
            }
            if (state == READY && event == BUTTON_EVENT_PRESSED &&
                snapshot.guidance_valid && health.brake_acknowledged)
            {
                if (s2_reset_stop() >= E_OK)
                {
                    health.stopped = false;
                    direction = 0;
                    visible_streak = 0u;
                    transition(&state, FOLLOW_LINE);
                }
                else
                {
                    stop(&state);
                }
            }
            if (state == FOLLOW_LINE || state == RECOVER_LINE)
            {
                bool usable = snapshot.normalized_valid &&
                    snapshot.capture_kind == IR_CAPTURE_NONE;
                if (!usable || health.stopped || (state == RECOVER_LINE &&
                    (uint32_t)(now - recovery_started) >= RECOVERY_MS))
                {
                    stop(&state);
                }
                else
                {
                    if (state == FOLLOW_LINE && snapshot.line == IR_LINE_LOST)
                    {
                        if (direction == 0)
                        {
                            stop(&state);
                        }
                        else
                        {
                            recovery_started = now;
                            visible_streak = 0u;
                            previous_sequence = snapshot.sequence;
                            previous_sample = snapshot.sampled_ms;
                            transition(&state, RECOVER_LINE);
                        }
                    }
                    if (state == RECOVER_LINE)
                    {
                        if (snapshot.line != IR_LINE_VISIBLE)
                        {
                            visible_streak = 0u;
                        }
                        else if (snapshot.guidance_valid &&
                            snapshot.sequence != previous_sequence &&
                            snapshot.sampled_ms != previous_sample)
                        {
                            ++visible_streak;
                            if (visible_streak >= 2u)
                            {
                                transition(&state, FOLLOW_LINE);
                            }
                        }
                        previous_sequence = snapshot.sequence;
                        previous_sample = snapshot.sampled_ms;
                    }
                    MotionRequest request = {.sampled_ms = snapshot.sampled_ms};
                    if (state == FOLLOW_LINE)
                    {
                        if (snapshot.line == IR_LINE_BROAD_BLACK)
                        {
                            request.left = BROAD_DUTY;
                            request.right = BROAD_DUTY;
                        }
                        else if (snapshot.guidance_valid)
                        {
                            float steer = snapshot.steering;
                            if (steer > MAX_STEER)
                            {
                                steer = MAX_STEER;
                            }
                            else if (steer < -MAX_STEER)
                            {
                                steer = -MAX_STEER;
                            }
                            float correction = BASE_DUTY * steer;
                            request.left = BASE_DUTY + correction;
                            request.right = BASE_DUTY - correction;
                            if (snapshot.steering != 0.0f)
                            {
                                direction = snapshot.steering > 0.0f ? 1 : -1;
                            }
                        }
                        else
                        {
                            stop(&state);
                        }
                    }
                    else if (state == RECOVER_LINE)
                    {
                        request.left = direction > 0 ? BASE_DUTY : 0.0f;
                        request.right = direction < 0 ? BASE_DUTY : 0.0f;
                    }
                    request.enabled = state == FOLLOW_LINE ||
                        state == RECOVER_LINE;
                    /* User tuning must stay within the existing S2 contract. */
                    if (request.left < 0.0f)
                    {
                        request.left = 0.0f;
                    }
                    else if (request.left > MOTION_MAX_DUTY)
                    {
                        request.left = MOTION_MAX_DUTY;
                    }
                    if (request.right < 0.0f)
                    {
                        request.right = 0.0f;
                    }
                    else if (request.right > MOTION_MAX_DUTY)
                    {
                        request.right = MOTION_MAX_DUTY;
                    }
                    if (request.enabled && s2_publish(request) < E_OK)
                    {
                        stop(&state);
                    }
                }
            }
        }
        next += FOLLOW_PERIOD_MS;
        now = robot_time_ms();
        if ((int32_t)(now - next) >= 0)
        {
            next = now + FOLLOW_PERIOD_MS;
        }
        if (tk_dly_tsk((RELTIM)(next - now)) < E_OK)
        {
            stop(&state);
            for (;;) tk_slp_tsk(TMO_FEVR); /* Halt: never return from a task */
        }
    }
}

ER line_follow_start(void)
{
    ID id = app_start_task((FP)follow_task, FOLLOW_PRIORITY,
        TP_PRC2, "Line follow");
    return id < E_OK ? id : E_OK;
}

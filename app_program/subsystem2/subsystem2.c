#include "subsystem2.h"
#include "drv_motor.h"
#include "../common/robot_time.h"
#include <math.h>
static ID request_mutex, stop_flag, motion_tid;
static MotionCommand latest;
static uint32_t request_sequence, copied_sequence;
static MotionController control;
static MotionFeedback feedback;
static MotionHealth progress;
static uint32_t control_epoch = 1u, brake_epoch;
static bool brake_acknowledged;

ER s2_stop(void)
{
    if (stop_flag <= 0)
    {
        return E_OBJ;
    }
    UW state;
    DI(state);
    ++control_epoch;
    brake_acknowledged = false;
    ER er = tk_set_flg(stop_flag, 1u);
    EI(state);
    if (motion_tid > 0)
    {
        (void)tk_wup_tsk(motion_tid);
    }
    return er;
}
ER s2_publish(MotionRequest request)
{
    (void)request;
    return E_NOSPT;
}
ER s2_command(const MotionCommand *request)
{
    if (request == NULL || tk_get_prc() != 2)
    {
        return E_PAR;
    }
    uint32_t now;
    if (!robot_now_ms(&now) ||
        (request->mode != MOTION_DUTY_BENCH && !robot_config_motion_valid()) ||
        !motion_command_valid(request, now, control_epoch, &robot_config))
    {
        return E_PAR;
    }
    ER er = tk_loc_mtx(request_mutex, TMO_POL);
    if (er < E_OK)
    {
        return er;
    }
    latest = *request;
    ++request_sequence;
    return tk_unl_mtx(request_mutex);
}
ER s2_feedback_copy(MotionFeedback *out)
{
    if (out == NULL || request_mutex <= 0)
    {
        return E_PAR;
    }
    ER er = tk_loc_mtx(request_mutex, TMO_POL);
    if (er < E_OK)
    {
        return er;
    }
    *out = feedback;
    return tk_unl_mtx(request_mutex);
}
ER s2_health(MotionHealth *out)
{
    if (out == NULL || request_mutex <= 0)
    {
        return E_PAR;
    }
    ER er = tk_loc_mtx(request_mutex, TMO_POL);
    if (er < E_OK)
    {
        return er;
    }
    *out = progress;
    er = tk_unl_mtx(request_mutex);
    UW state;
    DI(state);
    T_RFLG flag = {0};
    ER flag_er = tk_ref_flg(stop_flag, &flag);
    out->stopped = flag_er < E_OK || (flag.flgptn & 1u) != 0u;
    out->brake_acknowledged =
        out->stopped && brake_acknowledged && brake_epoch == control_epoch;
    out->run_epoch = control_epoch;
    EI(state);
    return er < E_OK ? er : flag_er;
}
ER s2_ack(uint32_t id)
{
    if (tk_get_prc() != 2)
    {
        return E_CTX;
    }
    return motion_control_ack(&control, id) ? E_OK : E_OBJ;
}
ER s2_prepare_reset(void)
{
    MotionHealth health;
    if (tk_get_prc() != 2 || s2_health(&health) < E_OK || !health.stopped ||
        !health.brake_acknowledged)
    {
        return E_OBJ;
    }
    return drv_encoder_rearm() ? E_OK : E_SYS;
}
ER s2_reset_stop(void)
{
    if (tk_get_prc() != 2)
    {
        return E_CTX;
    }
    ER er = tk_loc_mtx(request_mutex, TMO_POL);
    if (er < E_OK)
    {
        return er;
    }
    UW state;
    DI(state);
    T_RFLG flag = {0};
    er = tk_ref_flg(stop_flag, &flag);
    if (er >= E_OK)
    {
        if ((flag.flgptn & 1u) == 0u || !brake_acknowledged ||
            brake_epoch != control_epoch)
        {
            er = E_OBJ;
        }
        else
        {
            er = tk_clr_flg(stop_flag, 0u);
            if (er >= E_OK)
            {
                ++control_epoch;
                latest = (MotionCommand){0};
                copied_sequence = request_sequence;
                motion_control_init(&control, control_epoch);
                brake_acknowledged = false;
            }
        }
    }
    EI(state);
    ER unlock = tk_unl_mtx(request_mutex);
    return er < E_OK ? er : unlock;
}
static void motion_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    bool encoder_ready = drv_encoder_init();
    uint32_t previous_ms = 0u, next = 0u, stationary_since = 0u;
    uint32_t previous_left = 0u, previous_right = 0u, previous_epoch = 0u;
    MotionFeedback sample = {0};
    bool have_previous = false;
#if APP_TEST
    uint32_t report_due = 0u;
#endif
    for (;;)
    {
        uint32_t now;
        if (!robot_now_ms(&now))
        {
            (void)s2_stop();
            return;
        }
        EncoderCapture encoder = {0};
        bool acquired = encoder_ready && drv_encoder_copy(&encoder);
        if (acquired && encoder.epoch != previous_epoch)
        {
            have_previous = false;
            sample = (MotionFeedback){0};
            previous_epoch = encoder.epoch;
        }
        bool first_sample = !have_previous;
        uint32_t elapsed = have_previous ? now - previous_ms : 20u;
        if (elapsed == 0u)
        {
            elapsed = 1u;
        }
        float dt = (float)elapsed / 1000.0f;
        bool raw_quiet = have_previous && encoder.left_count == previous_left &&
                         encoder.right_count == previous_right;
        float dl = have_previous
                       ? (float)(int32_t)(encoder.left_count - previous_left) *
                             robot_config.left_m_per_count
                       : 0.0f;
        float dr =
            have_previous
                ? (float)(int32_t)(encoder.right_count - previous_right) *
                      robot_config.right_m_per_count
                : 0.0f;
        previous_left = encoder.left_count;
        previous_right = encoder.right_count;
        previous_ms = now;
        have_previous = true;
        sample.left_m += dl;
        sample.right_m += dr;
        sample.left_mps = dl / dt;
        sample.right_mps = dr / dt;
        if (robot_config.track_m > 0.0f)
        {
            float yaw = (dr - dl) / robot_config.track_m;
            float heading = sample.yaw_deg * 0.0174532925f + yaw * 0.5f;
            sample.x_m += (dl + dr) * 0.5f * cosf(heading);
            sample.y_m += (dl + dr) * 0.5f * sinf(heading);
            sample.yaw_deg += yaw * 57.2957795f;
        }
        sample.valid =
            acquired && robot_config_motion_valid() && elapsed <= 60u;
        bool quiet = !first_sample && acquired && raw_quiet;
        if (!quiet)
        {
            stationary_since = now;
        }
        sample.encoder_quiet =
            acquired && quiet && now - stationary_since >= 60u;
        sample.stationary =
            sample.valid && quiet && now - stationary_since >= 60u;
        sample.encoder = encoder;
        sample.sampled_ms = now;
        ++sample.sequence;
        MotionCommand request = {0};
        uint32_t sequence = copied_sequence;
        ER locked = tk_loc_mtx(request_mutex, TMO_POL);
        if (locked >= E_OK)
        {
            request = latest;
            sequence = request_sequence;
            (void)tk_unl_mtx(request_mutex);
        }
        uint32_t calculation_epoch = control_epoch;
        T_RFLG flag = {0};
        bool stopped =
            tk_ref_flg(stop_flag, &flag) < E_OK || (flag.flgptn & 1u) != 0u;
        float left = 0.0f, right = 0.0f;
        if (!stopped)
        {
            bool bench = request.mode == MOTION_DUTY_BENCH ||
                         control.command.mode == MOTION_DUTY_BENCH;
            if ((!sample.valid && !(bench && acquired && elapsed <= 60u)) ||
                locked < E_OK)
            {
                control.status = MOTION_FAILED;
                control.reason = MOTION_ENCODER_FAULT;
            }
            else
            {
                if (sequence != copied_sequence)
                {
                    copied_sequence = sequence;
                    if (!motion_control_accept(&control, &request,
                                               sample.left_m, sample.right_m,
                                               now, &robot_config))
                    {
                        control.status = MOTION_FAILED;
                        control.reason = MOTION_BAD_COMMAND;
                    }
                }
                motion_control_step(&control, sample.left_m, sample.right_m,
                                    sample.left_mps, sample.right_mps, dt, now,
                                    &robot_config, &left, &right);
            }
        }
        UW state;
        DI(state);
        flag = (T_RFLG){0};
        stopped = tk_ref_flg(stop_flag, &flag) < E_OK ||
                  (flag.flgptn & 1u) != 0u ||
                  calculation_epoch != control_epoch;
        if (control.status == MOTION_FAILED && !stopped)
        {
            (void)tk_set_flg(stop_flag, 1u);
            ++control_epoch;
            stopped = true;
        }
        ER output;
#if APP_TEST
        output = drv_motor_brake();
#else
        output = stopped ? drv_motor_brake() : drv_motor_apply(left, right);
#endif
        if (output < E_OK)
        {
            (void)tk_set_flg(stop_flag, 1u);
            stopped = true;
            brake_acknowledged = false;
            (void)drv_motor_brake();
        }
        else
        {
            brake_acknowledged = stopped;
            brake_epoch = control_epoch;
        }
        EI(state);
        sample.left_duty = APP_TEST || stopped ? 0.0f : left;
        sample.right_duty = APP_TEST || stopped ? 0.0f : right;
        sample.run_epoch = control_epoch;
        sample.action_id = control.command.action_id;
        sample.status = control.status;
        sample.reason = control.reason;
        if (tk_loc_mtx(request_mutex, TMO_POL) >= E_OK)
        {
            feedback = sample;
            if (output >= E_OK)
            {
                progress.serviced_ms = now;
                ++progress.sequence;
            }
            (void)tk_unl_mtx(request_mutex);
        }
        else
        {
            (void)s2_stop();
        }
#if APP_TEST
        if (robot_due(now, report_due))
        {
            tm_printf((UB *)"[MOTION] count L=%u R=%u faults=%u valid=%d "
                            "stationary=%d MOTOR_INHIBITED\n",
                      (unsigned)encoder.left_count,
                      (unsigned)encoder.right_count, (unsigned)encoder.faults,
                      sample.valid, sample.stationary);
            report_due = now + 500u;
        }
#endif
        if (!robot_now_ms(&now))
        {
            (void)s2_stop();
            return;
        }
        if (next == 0u)
        {
            next = now + 20u;
        }
        if (robot_due(now, next))
        {
            next += 20u;
            if (robot_due(now, next))
            {
                next = now + 20u;
            }
        }
        (void)tk_slp_tsk((TMO)(next - now));
        (void)tk_can_wup(TSK_SELF);
    }
}
ER subsystem2_init(void)
{
    ER er = drv_motor_init();
    if (er < E_OK)
    {
        return er;
    }
    T_CMTX mutex = {0};
    mutex.mtxatr = TA_INHERIT;
    request_mutex = tk_cre_mtx(&mutex);
    if (request_mutex <= 0)
    {
        return request_mutex;
    }
    T_CFLG flag = {0};
    flag.iflgptn = 1u;
    stop_flag = tk_cre_flg(&flag);
    motion_control_init(&control, control_epoch);
    return stop_flag < E_OK ? stop_flag : E_OK;
}
ER subsystem2_start(void)
{
    motion_tid =
        app_start_task((FP)motion_task, S2_PRI_MOTION, TP_PRC2, "Motion");
    return motion_tid < E_OK ? motion_tid : E_OK;
}

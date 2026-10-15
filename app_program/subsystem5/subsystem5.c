#include "subsystem5.h"
#include "../common/drv_gpio_irq.h"
#include "../common/robot_config.h"
#include "../common/robot_time.h"
#include "../subsystem2/subsystem2.h"
#include <stdatomic.h>
#include <math.h>
#define ECHO_PIN 2u
#define TRIGGER_PIN 3u
#define SERVO_PIN 12u
static ID snapshot_mutex;
static ObstacleSnapshot latest;
static uint32_t requested_id, requested_deadline;
static bool have_request;
static atomic_bool cancelled, reset_pending;
static volatile uint32_t rise_us, pulse_us;
static volatile unsigned echo_state;

static void echo_edge(unsigned pin, uint32_t edges, uint32_t now)
{
    (void)pin;
    if (echo_state == 0u || echo_state >= 3u)
    {
        return;
    }
    if (edges == 12u)
    {
        echo_state = 4u;
    }
    else if (edges == 8u && echo_state == 1u)
    {
        rise_us = now;
        echo_state = 2u;
    }
    else if (edges == 4u && echo_state == 2u)
    {
        pulse_us = now - rise_us;
        echo_state = pulse_us > 0u && pulse_us < 30000u ? 3u : 4u;
    }
    else
    {
        echo_state = 4u;
    }
}
static bool servo_angle(float angle)
{
    float pulse =
        (float)robot_config.servo_center_us +
        angle * robot_config.servo_us_per_deg * robot_config.servo_left_sign;
    if (!isfinite(pulse) || pulse < robot_config.servo_min_us ||
        pulse > robot_config.servo_max_us)
    {
        return false;
    }
    return pwm_set_cc(SERVO_PIN, (UW)pulse) >= E_OK;
}
static bool hardware_init(bool configured)
{
    if (drv_gpio_irq_init() < E_OK || gpio_set_val(TRIGGER_PIN, 0u) < E_OK ||
        gpio_set_pin(TRIGGER_PIN, GPIO_MODE_OUT) < E_OK ||
        gpio_set_pin(ECHO_PIN, GPIO_MODE_IN) < E_OK ||
        !drv_gpio_irq_claim(ECHO_PIN, echo_edge) ||
        !drv_gpio_irq_enable(ECHO_PIN, true))
    {
        return false;
    }
    if (!configured)
    {
        return true;
    }
    clr_w(RESETS_RESET, RESETS_RESET_PWM);
    out_w(PWM_CH6_BASE + PWM_CHx_CSR, 0u);
    out_w(PWM_CH6_BASE + PWM_CHx_CTR, 0u);
    out_w(PWM_CH6_BASE + PWM_CHx_DIV, 125u * 16u);
    return pwm_set_wrap(SERVO_PIN, 19999u) >= E_OK &&
           pwm_set_cc(SERVO_PIN, 0u) >= E_OK &&
           pwm_set_pin(SERVO_PIN) >= E_OK && servo_angle(0.0f) &&
           pwm_set_enabled(SERVO_PIN, TRUE) >= E_OK;
}
static bool trigger(void)
{
    if (gpio_get_val(ECHO_PIN) != 0)
    {
        return false;
    }
    UW state;
    DI(state);
    echo_state = 1u;
    EI(state);
    bool pulse_ok =
        gpio_set_val(TRIGGER_PIN, 0u) >= E_OK && robot_pulse_wait_us(2u) &&
        gpio_set_val(TRIGGER_PIN, 1u) >= E_OK && robot_pulse_wait_us(10u);
    /* Always restore low, including a failed timing/driver operation. */
    bool low = gpio_set_val(TRIGGER_PIN, 0u) >= E_OK;
    if (!pulse_ok || !low)
    {
        DI(state);
        echo_state = 0u;
        EI(state);
        return false;
    }
    return true;
}
ER s5_obstacle_copy(ObstacleSnapshot *out)
{
    if (out == NULL || snapshot_mutex <= 0)
    {
        return E_PAR;
    }
    ER er = tk_loc_mtx(snapshot_mutex, TMO_POL);
    if (er < E_OK)
    {
        return er;
    }
    *out = latest;
    return tk_unl_mtx(snapshot_mutex);
}
ER s5_scan_request(uint32_t id, uint32_t deadline)
{
    uint32_t now;
    if (id == 0u || !robot_now_ms(&now) || !robot_config_sonar_valid() ||
        robot_due(now, deadline) || deadline - now > robot_config.scan_ms)
    {
        return E_PAR;
    }
    MotionFeedback motion;
    if (s2_feedback_copy(&motion) < E_OK || !motion.valid ||
        !motion.stationary || now - motion.sampled_ms > 60u)
    {
        return E_OBJ;
    }
    ER er = tk_loc_mtx(snapshot_mutex, TMO_POL);
    if (er < E_OK)
    {
        return er;
    }
    if (id == latest.scan_id || (have_request && id == requested_id))
    {
        er = E_OK;
    }
    else if (have_request || latest.scan_status == SCAN_RUNNING)
    {
        er = E_OBJ;
    }
    else
    {
        requested_id = id;
        requested_deadline = deadline;
        have_request = true;
        atomic_store_explicit(&cancelled, false, memory_order_release);
    }
    ER unlock = tk_unl_mtx(snapshot_mutex);
    return er < E_OK ? er : unlock;
}
void s5_scan_cancel(void)
{
    atomic_store_explicit(&cancelled, true, memory_order_release);
}
void s5_reset_request(void)
{
    atomic_store_explicit(&reset_pending, true, memory_order_release);
}
static void obstacle_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    ObstacleSnapshot s = {.configured = robot_config_sonar_valid()};
    bool hardware = hardware_init(s.configured);
    float angles[S5_SCAN_CAPACITY] = {-60.0f, -30.0f, 0.0f, 30.0f, 60.0f};
    unsigned index = 0u, total = 5u;
    bool scanning = false, waiting = false, restoring = false;
    uint32_t settle_until = 0u, next_trigger = 0u, trigger_us = 0u;
    uint32_t deadline = 0u;
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
        if (atomic_load_explicit(&reset_pending, memory_order_acquire))
        {
            atomic_store_explicit(&reset_pending, false, memory_order_release);
            MotionHealth health;
            if (s2_health(&health) >= E_OK && health.stopped &&
                health.brake_acknowledged)
            {
                hardware = hardware_init(s.configured);
                scanning = waiting = false;
                restoring = true;
                echo_state = 0u;
                s.front_valid = s.profile_valid = false;
                s.scan_status = SCAN_IDLE;
                settle_until = now + robot_config.servo_settle_ms;
            }
        }
        if (settle_until == 0u)
        {
            settle_until = now + robot_config.servo_settle_ms;
        }
        if (tk_loc_mtx(snapshot_mutex, TMO_POL) >= E_OK)
        {
            if (have_request && !scanning && !restoring)
            {
                have_request = false;
                scanning = true;
                s.scan_id = requested_id;
                deadline = requested_deadline;
                s.scan_status = SCAN_RUNNING;
                s.profile_valid = false;
                s.front_valid = false;
                s.ray_count = 0u;
                index = 0u;
                total = 5u;
                waiting = false;
                echo_state = 0u;
                if (!servo_angle(angles[0]))
                {
                    s.scan_status = SCAN_FAILED;
                    scanning = false;
                    restoring = true;
                    (void)servo_angle(0.0f);
                }
                settle_until = now + robot_config.servo_settle_ms;
            }
            (void)tk_unl_mtx(snapshot_mutex);
        }
        MotionFeedback motion = {0};
        bool stationary = s2_feedback_copy(&motion) >= E_OK && motion.valid &&
                          motion.stationary && now - motion.sampled_ms <= 60u;
        bool cancel_scan =
            atomic_load_explicit(&cancelled, memory_order_acquire);
        bool active_scan =
            scanning || (restoring && s.scan_status != SCAN_IDLE &&
                         s.scan_status != SCAN_CANCELLED);
        if (active_scan &&
            (cancel_scan || (s.scan_status == SCAN_RUNNING &&
                             (robot_due(now, deadline) || !stationary))))
        {
            s.scan_status = s5_scan_guard(cancel_scan, robot_due(now, deadline),
                                          stationary);
            s.profile_valid = false;
            scanning = false;
            restoring = true;
            waiting = false;
            echo_state = 0u;
            hardware = hardware && servo_angle(0.0f);
            settle_until = now + robot_config.servo_settle_ms;
        }
        if (hardware && !waiting && robot_due(now, settle_until) &&
            robot_due(now, next_trigger))
        {
            trigger_us = in_w(TIMER_TIMERAWL);
            waiting = trigger();
            next_trigger = now + robot_config.sonar_repeat_ms;
            if (!waiting)
            {
                s.front_valid = false;
                if (scanning)
                {
                    s.scan_status = SCAN_FAILED;
                    scanning = false;
                    restoring = true;
                    hardware = servo_angle(0.0f);
                    settle_until = now + robot_config.servo_settle_ms;
                }
            }
        }
        if (waiting && (echo_state >= 3u || (uint32_t)(in_w(TIMER_TIMERAWL) -
                                                       trigger_us) >= 30000u))
        {
            waiting = false;
            float range = (float)pulse_us / 5800.0f;
            bool valid = echo_state == 3u && isfinite(range) && s.configured &&
                         range >= robot_config.sonar_min_m &&
                         range <= robot_config.sonar_max_m;
            s.echo_us = echo_state == 3u ? pulse_us : 0u;
            s.echo_valid = echo_state == 3u;
            echo_state = 0u;
            ++s.sequence;
            s.progress_ms = now;
            if (scanning)
            {
                s.rays[index] = (ObstacleRay){angles[index], range, now, valid};
                s.ray_count = (uint8_t)(index + 1u);
                if (++index == 5u)
                {
                    unsigned nearest = 0u;
                    for (unsigned i = 1u; i < 5u; ++i)
                    {
                        if (s.rays[i].valid &&
                            s.rays[i].range_m < s.rays[nearest].range_m)
                        {
                            nearest = i;
                        }
                    }
                    static const float offsets[4] = {-20.0f, -10.0f, 10.0f,
                                                     20.0f};
                    for (unsigned i = 0u; i < 4u; ++i)
                    {
                        float angle = angles[nearest] + offsets[i];
                        angles[5u + i] = fmaxf(-60.0f, fminf(angle, 60.0f));
                    }
                    total = S5_SCAN_CAPACITY;
                }
                if (index >= total)
                {
                    s.profile_valid = s5_profile_build(&s);
                    s.profile_ms = now;
                    s.scan_status =
                        s.profile_valid ? SCAN_RUNNING : SCAN_FAILED;
                    scanning = false;
                    restoring = true;
                    hardware = servo_angle(0.0f);
                    settle_until = now + robot_config.servo_settle_ms;
                }
                else if (!servo_angle(angles[index]))
                {
                    scanning = false;
                    restoring = true;
                    s.profile_valid = false;
                    s.scan_status = SCAN_FAILED;
                    hardware = servo_angle(0.0f);
                    settle_until = now + robot_config.servo_settle_ms;
                }
                else
                {
                    settle_until = now + robot_config.servo_settle_ms;
                }
            }
            else
            {
                s.front_m = range;
                s.front_ms = now;
                s.front_valid = valid;
                if (restoring)
                {
                    restoring = false;
                    if (s.scan_status == SCAN_RUNNING)
                    {
                        s.scan_status =
                            valid && s.profile_valid ? SCAN_DONE : SCAN_FAILED;
                    }
                }
            }
        }
        if (now - s.front_ms > 250u || scanning || restoring)
        {
            s.front_valid = false;
        }
        if (!hardware)
        {
            s.front_valid = s.profile_valid = false;
        }
        if (tk_loc_mtx(snapshot_mutex, TMO_POL) >= E_OK)
        {
            latest = s;
            (void)tk_unl_mtx(snapshot_mutex);
        }
#if APP_TEST
        if (robot_due(now, report_due))
        {
            tm_printf((UB *)"[S5] echo_us=%u echo_available=%d front_valid=%d "
                            "configured=%d scan=%u\n",
                      (unsigned)s.echo_us, s.echo_valid, s.front_valid,
                      s.configured, (unsigned)s.scan_status);
            report_due = now + 500u;
        }
#endif
        if (tk_dly_tsk(2u) < E_OK)
        {
            (void)s2_stop();
            return;
        }
    }
}
ER subsystem5_init(void)
{
    T_CMTX mutex = {0};
    mutex.mtxatr = TA_CEILING;
    mutex.ceilpri = APP_SHARED_MUTEX_CEILING;
    snapshot_mutex = tk_cre_mtx(&mutex);
    if (snapshot_mutex <= 0)
    {
        return snapshot_mutex;
    }
    if (gpio_set_val(12u, 0u) < E_OK || gpio_set_pin(12u, GPIO_MODE_OUT) < E_OK)
    {
        return E_IO;
    }
    return E_OK;
}
ER subsystem5_start(void)
{
    ID tid =
        app_start_task((FP)obstacle_task, S5_PRI_OBSTACLE, TP_PRC2, "Obstacle");
    return tid < E_OK ? tid : E_OK;
}

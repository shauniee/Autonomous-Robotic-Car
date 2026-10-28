#include "safety.h"
#include <stdatomic.h>
#include "subsystem3.h"
#include "../subsystem2/subsystem2.h"
#include "../common/buttons.h"
#include "../common/robot_time.h"

static rd_word_t latched;		/* writer: Safety task */
/* Cortex-M0+ has no atomic read-modify-write: one writer per counter. */
static _Atomic uint32_t reset_requests;	/* writer: Mission */
static _Atomic uint32_t reset_served;	/* writer: Safety */

uint32_t safety_latched_causes(void)
{
    return rd_word_read(&latched);
}

void safety_request_reset(void)
{
    atomic_store_explicit(&reset_requests,
        atomic_load_explicit(&reset_requests, memory_order_relaxed) + 1u, memory_order_release);
}

static void safety_task(INT stacd, void *exinf)
{
    (void)stacd; (void)exinf;
    uint32_t release = robot_time_ms();
    for (;;)
    {
        uint32_t now = robot_time_ms();
        ButtonInputs buttons = drv_buttons_read();
        S3IrSnapshot snapshot = {0};
        MotionHealth health = {0};
        bool line_copied = s3_ir_copy(&snapshot);
        bool health_read = s2_health(&health) >= E_OK;

        SafetyInputs in = {
            .stop_button = buttons.gp21_down,
            .fault_pending = rd_fault_stop_pending(),
            /* A busy copy keeps the previous judgement: age only what was read. */
            .line_seen = line_copied && snapshot.sampled_ms != 0u,
            .line_age_ms = line_copied ? robot_time_elapsed(now, snapshot.sampled_ms) : 0u,
            .motion_seen = health_read && health.sequence != 0u,
            .motion_age_ms = health_read ? robot_time_elapsed(now, health.serviced_ms) : 0u,
            .uptime_ms = now};
        uint32_t causes = safety_causes(&in);
        uint32_t held = rd_word_read(&latched);
        uint32_t requested = atomic_load_explicit(&reset_requests, memory_order_acquire);
        bool reset_pending = requested != atomic_load_explicit(&reset_served, memory_order_relaxed);

        if (causes != 0u)
        {
            if (held == 0u)
            {
                (void)s2_stop(); /* once per latch: repeats would reset the brake acknowledgement */
                tm_printf((UB *)"[SAFETY] latched causes=0x%x (1 GP21, 2 fault, 4 stale line, 8 stale motion)"
                          " line_seen=%d line_age=%u motion_seen=%d motion_age=%u up=%u\n",
                          (unsigned)causes, (int)in.line_seen, (unsigned)in.line_age_ms,
                          (int)in.motion_seen, (unsigned)in.motion_age_ms, (unsigned)now);
            }
            rd_word_publish(&latched, held | causes);
        }
        else if (reset_pending && (held != 0u || (health_read && motion_health_stopped(&health))) &&
                 safety_reset_allowed(causes, health_read && motion_health_brake_acknowledged(&health)))
        {
            if (s2_reset_stop() >= E_OK)
            {
                rd_word_publish(&latched, 0u);
            }
        }
        /* A request is consumed on the release it is seen: a refused reset needs a new press. */
        atomic_store_explicit(&reset_served, requested, memory_order_release);
        tk_dly_tsk(robot_time_next_delay(&release, SAFETY_PERIOD_MS, robot_time_ms()));
    }
}

ER safety_start(void)
{
    ID tid = app_start_task((FP)safety_task, SAFETY_PRI, TP_PRC2, "Safety");
    return tid < E_OK ? tid : E_OK;
}

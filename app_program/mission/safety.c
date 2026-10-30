#include "safety.h"
#include "../subsystem3/subsystem3.h"
#include "../subsystem2/subsystem2.h"
#include "../common/buttons.h"
#include "../common/robot_time.h"

static rd_word_t latched; /* writer: Mission; readers use one-word copies */
static Button reset_button;
static bool initialized;
static uint8_t reset_state = SAFETY_RESET_BOOT;

uint32_t safety_latched_causes(void)
{
    return rd_word_read(&latched);
}

/* Bounded policy step on core 1. Motion independently checks GP21, faults,
   command expiry and action failure before writing the motor outputs. */
void safety_step(bool line_required)
{
    uint32_t now = robot_time_ms();
    ButtonInputs buttons = drv_buttons_read();
    if (!initialized)
    {
        button_init(&reset_button, now);
        initialized = true;
    }
    ButtonEvent event = button_update(&reset_button, buttons.gp21_down, now);
    S3IrSnapshot snapshot = {0};
    MotionHealth health = {0};
    bool line_copied = s3_ir_copy(&snapshot);
    bool health_read = s2_health(&health) >= E_OK;
    SafetyInputs in = {
        .stop_button = buttons.gp21_down,
        .line_required = line_required,
        .fault_pending = rd_fault_stop_pending(),
        .line_seen = line_copied && snapshot.sampled_ms != 0u,
        .line_age_ms = line_copied ?
            robot_time_elapsed(now, snapshot.sampled_ms) : 0u,
        .motion_seen = health_read && health.sequence != 0u,
        .motion_age_ms = health_read ?
            robot_time_elapsed(now, health.serviced_ms) : 0u,
        .uptime_ms = now};
    uint32_t causes = safety_causes(&in);
    uint32_t held = rd_word_read(&latched);
    if (causes != 0u)
    {
        /* Capture transient owner faults before Motion clears them at rest. */
        uint8_t motion_faults = rd_fault_get(RD_OWNER_MOTION);
        uint8_t terrain_faults = rd_fault_get(RD_OWNER_TERRAIN);
        if (held == 0u)
        {
            (void)s2_stop();
        }
        rd_word_publish(&latched, held | causes);
        if ((causes & ~held) != 0u)
        {
            /* Diagnostics follow the stop request and latch publication. */
            tm_printf((UB *)"[SAFETY] causes=%u gp21=%d fault=%d "
                "line_stale=%d motion_stale=%d motion_faults=%u "
                "terrain_faults=%u line_age_ms=%u motion_age_ms=%u\n",
                (unsigned)causes, (int)in.stop_button,
                (int)((causes & SAFETY_CAUSE_FAULT) != 0u),
                (int)((causes & SAFETY_CAUSE_STALE_LINE) != 0u),
                (int)((causes & SAFETY_CAUSE_STALE_MOTION) != 0u),
                (unsigned)motion_faults, (unsigned)terrain_faults,
                (unsigned)in.line_age_ms, (unsigned)in.motion_age_ms);
        }
    }
    if (safety_reset_step(&reset_state, event, causes,
            health_read && motion_health_brake_acknowledged(&health)))
    {
        if (s2_reset_stop() >= E_OK)
        {
            rd_word_publish(&latched, 0u);
            reset_state = SAFETY_RESET_PRESS;
        }
    }
}

#include "io_task.h"
#include "robot_time.h"
#include "../comms/comms.h"
#include "../terrain/terrain.h"
#include "robot_watchdog.h"

#if ROBOT_WATCHDOG_ENABLED
#include <stdatomic.h>

/* RP2040 register map (not a Pico SDK hardware_watchdog linkage). The existing
 * app_timebase_init() sets WATCHDOG.TICK to 1 MHz before tasks start.
 * RP2040-E1 halves the apparent timeout: load two ticks per microsecond.
 * Watchdog resets both processors and resettable peripherals, including PWM.
 * Physical motor-driver behaviour through the reset must still be verified.
 */
#define ROBOT_WD_CTRL       0x40058000u
#define ROBOT_WD_LOAD       0x40058004u
#define ROBOT_WD_REASON     0x40058008u
#define ROBOT_WD_SCRATCH4   0x4005801cu
#define ROBOT_PSM_WDSEL     0x40010008u
#define ROBOT_RESETS_WDSEL  0x4000c004u
#define ROBOT_WD_ENABLE     (1u << 30)
#define ROBOT_WD_DEBUG_PAUSE (7u << 24)
#define ROBOT_WD_RESETS     0x01ffffffu
#define ROBOT_WD_PSM_RESETS 0x0001fffcu /* excludes ROSC and XOSC */
#define ROBOT_WD_MAGIC      0x6ab73121u
#define ROBOT_WD_RELOAD     (ROBOT_WATCHDOG_TIMEOUT_MS * 2000u)

static _Atomic uint32_t motion_progress_ms;
static _Atomic uint32_t mission_progress_ms;

void robot_watchdog_note_motion(uint32_t serviced_ms)
{
    atomic_store_explicit(&motion_progress_ms, serviced_ms, memory_order_release);
}

void robot_watchdog_note_mission(uint32_t serviced_ms)
{
    atomic_store_explicit(&mission_progress_ms, serviced_ms, memory_order_release);
}

/* Sole hardware feeder. Both other tasks must make fresh, *new* progress
 * since the previous feed. No network callback, timer IRQ or unrelated
 * service may refresh the watchdog.
 */
void robot_watchdog_io_service(uint32_t now_ms)
{
    static bool armed, boot_reason_reported;
    static uint32_t last_fed_motion, last_fed_mission;
    uint32_t motion = atomic_load_explicit(&motion_progress_ms, memory_order_acquire);
    uint32_t mission = atomic_load_explicit(&mission_progress_ms, memory_order_acquire);
    bool fresh = robot_watchdog_progress_fresh(now_ms, motion, mission,
        ROBOT_WATCHDOG_PROGRESS_MAX_AGE_MS);

    if (!boot_reason_reported)
    {
        boot_reason_reported = true;
        if ((in_w(ROBOT_WD_REASON) & 1u) != 0u &&
            in_w(ROBOT_WD_SCRATCH4) == ROBOT_WD_MAGIC)
        {
            tm_printf((UB *)"[WATCHDOG] previous boot reset by watchdog timeout; "
                "motor startup latch must be checked\n");
        }
    }
    if (!armed)
    {
        /* Defer arming until Mission and Motion are actually running; TEST
         * firmware starts Mission after its initial self-test procedures. */
        if (!fresh) return;
        out_w(ROBOT_RESETS_WDSEL, ROBOT_WD_RESETS);
        out_w(ROBOT_PSM_WDSEL, ROBOT_WD_PSM_RESETS);
        out_w(ROBOT_WD_SCRATCH4, ROBOT_WD_MAGIC);
        out_w(ROBOT_WD_LOAD, ROBOT_WD_RELOAD);
        out_w(ROBOT_WD_CTRL, ROBOT_WD_ENABLE | ROBOT_WD_DEBUG_PAUSE);
        last_fed_motion = motion;
        last_fed_mission = mission;
        armed = true;
        tm_printf((UB *)"[WATCHDOG] ARMED timeout_ms=%u; "
            "Motion/Mission/I-O progress required\n",
            (unsigned)ROBOT_WATCHDOG_TIMEOUT_MS);
        return;
    }
    if (robot_watchdog_should_feed(now_ms, motion, mission,
        last_fed_motion, last_fed_mission,
        ROBOT_WATCHDOG_PROGRESS_MAX_AGE_MS))
    {
        out_w(ROBOT_WD_LOAD, ROBOT_WD_RELOAD);
        last_fed_motion = motion;
        last_fed_mission = mission;
    }
    /* Otherwise do not feed: hardware times out even if core 0 runs. */
}
#endif /* ROBOT_WATCHDOG_ENABLED */


static void io_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    uint32_t release = robot_time_ms();
    for (;;)
    {
        uint32_t now = robot_time_ms();
        terrain_step(now);
        comms_step(robot_time_ms());
        robot_watchdog_io_service(robot_time_ms());
        (void)tk_dly_tsk(robot_time_next_delay(&release, IO_PERIOD_MS,
            robot_time_ms()));
    }
}

ER io_start(void)
{
    ID id = app_start_task((FP)io_task, IO_PRIORITY, TP_PRC1, "I/O");
    return id < E_OK ? id : E_OK;
}

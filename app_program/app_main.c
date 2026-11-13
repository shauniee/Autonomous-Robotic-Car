#include "app.h"
#include "motion/motion.h"
#include "mission/mission.h"
#include "common/io_task.h"
#include "common/task_timing.h"

/*
 * Create and start one task (declared in app.h for every subsystem).
 *   prc = TP_PRC1 (core 0), TP_PRC2 (core 1), or 0 for "any core".
 */
EXPORT ID app_start_task(FP entry, PRI pri, UW prc, const char *name)
{
    T_CTSK  ctsk = {0};
    ID  tid;
    ER  er;

    ctsk.task    = entry;       /* function the task runs */
    ctsk.itskpri = pri;     /* initial priority (1 = highest) */
    ctsk.stksz   = TASK_STACK_SIZE; /* kernel allocates the stack */
    ctsk.tskatr  = TA_HLNG      /* written in a high-level language (C) */
             | TA_RNG3;     /* runs at user protection level */
    /* Trace images use painted static stacks so [TIMING] can report use.
       task_timing_id_t follows the creation order in usermain(). */
    static unsigned created;
    if (created < TASK_TIMING_COUNT)
    {
        ctsk.bufptr = task_timing_stack((task_timing_id_t)created++,
            &ctsk.stksz);
        if (ctsk.bufptr != NULL)
        {
            ctsk.tskatr |= TA_USERBUF;
        }
    }
#if TK_SUPPORT_SMP
    if (prc != 0)
    {
        ctsk.tskatr |= TA_ASSPRC;   /* honour the affinity below */
        ctsk.assprc  = prc;     /* which core(s) it may run on */
    }
#else
    (void)prc;          /* single-core build: no affinity */
#endif

    /* tk_cre_tsk: create the task.  It starts in the DORMANT state. */
    tid = tk_cre_tsk(&ctsk);
    if (tid < E_OK)
    {
        CHECK(tid, name);
        return tid;
    }

    /* tk_sta_tsk: DORMANT -> READY.  The int is passed as stacd. */
    er = tk_sta_tsk(tid, 0);
    CHECK(er, name);
    return er < E_OK ? er : tid;
}

static void startup_error(void)
{
    motion_inhibit_startup();
    tm_printf((UB*)"Startup failed: motor outputs remain inhibited.\n");
    for (;;)
    {
        tk_slp_tsk(TMO_FEVR);
    }
}
#define REQUIRE(call) \
    do \
    { \
        ER er = (call); \
        CHECK(er, #call); \
        if (er < E_OK) \
        { \
            startup_error(); \
        } \
    } while (0)

/* Shared 1 us hardware TIMER tick from the 12 MHz reference. Scheduling uses
   SysTick. Release TIMER without resetting a timebase also read by WiFi. */
/* WATCHDOG.TICK: 1 us reference divider */
#define RP2040_WATCHDOG_TICK    0x4005802cu

static void app_timebase_init(void)
{
    const UW watchdog_tick_register = RP2040_WATCHDOG_TICK;
    const UW tick_enable = 1u << 9;
    const UW reference_cycles_per_us = 12u;

    out_w(watchdog_tick_register, tick_enable | reference_cycles_per_us);
    clr_w(RESETS_RESET, RESETS_RESET_TIMER);
}

EXPORT INT usermain(void)
{
    T_RVER ver; // version information from kernel
    tk_ref_ver(&ver); // request version information
    tm_printf((UB*)"\n=== micro T-Kernel on Raspberry Pi Pico W "
              "(spec 0x%04x, product 0x%04x) ===\n",
              ver.spver, ver.prver);
#if TK_SUPPORT_SMP // SMP -> symmetric multiprocessing
    tm_printf((UB*)"dual-core (SMP) build, usermain on core %d\n",
              tk_get_prc() - 1);
#else
    tm_printf((UB*)"single-core build\n");
#endif

    app_timebase_init(); // time in ms

    // every subsystem creates its kernel objects
    REQUIRE(motion_init());
    REQUIRE(comms_init());
    REQUIRE(line_barcode_init());
    REQUIRE(terrain_init());
    REQUIRE(obstacle_init());

    /* Exactly three application tasks. Producers initialize peripheral capture
       on their owning core; all kernel objects already exist. */
    REQUIRE(motion_start());
    REQUIRE(io_start());
#if !APP_TEST
    REQUIRE(mission_start());
#endif

#if APP_TEST
    tm_printf((UB*)"Motion and I/O started; "
              "Mission starts after startup tests.\n");
    tm_printf((UB*)"[TEST] === test run start ===\n");
    comms_test();
    motion_test();
    terrain_test();
    REQUIRE(mission_start()); /* Mission samples line IR, sonar */
    line_barcode_test();
    obstacle_test();
    tm_printf((UB*)"[TEST] === test run end ===\n");
#endif
    tm_printf((UB*)"all subsystems started\n\n");

    // sleep forever, returning from usermain() would shut down.
    for (;;)
    {
        tk_slp_tsk(TMO_FEVR);
    }
}

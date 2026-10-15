#include "app.h"
#include "subsystem2/subsystem2.h"
#include "subsystem3/mission.h"
#include "common/drv_buttons.h"
#include "common/button.h"
#include "common/robot_time.h"

/*
 * Create and start one task (declared in app.h for every subsystem).
 *   prc = TP_PRC1 (core 0), TP_PRC2 (core 1), or 0 for "any core".
 */
EXPORT ID app_start_task(FP entry, PRI pri, UW prc, const char *name)
{
    T_CTSK ctsk = {0};
    ID tid;
    ER er;

    ctsk.task = entry;            /* function the task runs */
    ctsk.itskpri = pri;           /* initial priority (1 = highest) */
    ctsk.stksz = TASK_STACK_SIZE; /* kernel allocates the stack */
    ctsk.tskatr = TA_HLNG         /* written in a high-level language (C) */
                  | TA_RNG3;      /* runs at user protection level */
#if TK_SUPPORT_SMP
    if (prc != 0)
    {
        ctsk.tskatr |= TA_ASSPRC; /* honour the affinity below */
        ctsk.assprc = prc;        /* which core(s) it may run on */
    }
#else
    (void)prc; /* single-core build: no affinity */
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
    /* Release is never published after a startup error; S2 boot latch holds. */
    tm_printf((UB *)"Startup failed: motor outputs remain inhibited.\n");
    for (;;)
        tk_slp_tsk(TMO_FEVR);
}
#define REQUIRE(call)                                                          \
    do                                                                         \
    {                                                                          \
        ER er = (call);                                                        \
        CHECK(er, #call);                                                      \
        if (er < E_OK)                                                         \
            startup_error();                                                   \
    } while (0)

/* Shared 1 us hardware TIMER tick from the 12 MHz reference. Scheduling uses
   SysTick. Release TIMER without resetting a timebase also read by WiFi. */
static void app_timebase_init(void)
{
    const UW watchdog_tick_register = 0x4005802cu;
    const UW tick_enable = 1u << 9;
    const UW reference_cycles_per_us = 12u;

    out_w(watchdog_tick_register, tick_enable | reference_cycles_per_us);
    clr_w(RESETS_RESET, RESETS_RESET_TIMER);
}

EXPORT INT usermain(void)
{
    T_RVER ver;       // version information from kernel
    tk_ref_ver(&ver); // request version information
    tm_printf((UB *)"\n=== micro T-Kernel on Raspberry Pi Pico W (spec 0x%04x, "
                    "product 0x%04x) ===\n",
              ver.spver, ver.prver);
#if TK_SUPPORT_SMP // SMP -> symmetric multiprocessing
    tm_printf((UB *)"dual-core (SMP) build, usermain on core %d\n",
              tk_get_prc() - 1);
#else
    tm_printf((UB *)"single-core build\n");
#endif

    app_timebase_init(); // time in ms

    // every subsystem creates its kernel objects
    REQUIRE(subsystem1_init());
    REQUIRE(subsystem2_init());
    REQUIRE(subsystem3_init());
    REQUIRE(subsystem4_init());
    REQUIRE(subsystem5_init());

    REQUIRE(mission_init());
#if APP_TEST
    Button gate;
    uint32_t now;
    (void)robot_now_ms(&now);
    button_init(&gate, now);
    bool pressed = false;
    tm_printf((UB *)"[TEST] release GP20, press and release to run inhibited "
                    "fixtures\n");
    for (;;)
    {
        (void)robot_now_ms(&now);
        ButtonInputs inputs = drv_buttons_read();
        ButtonEvent event = button_update(&gate, inputs.gp20_down, now);
        if (inputs.gp21_down)
        {
            pressed = false;
            button_disarm(&gate);
        }
        else if (event == BUTTON_EVENT_PRESSED)
        {
            pressed = true;
        }
        else if (pressed && event == BUTTON_EVENT_RELEASED)
        {
            break;
        }
        tk_dly_tsk(10u);
    }
    tm_printf((UB *)"[TEST] === test run start ===\n");
    subsystem1_test();
    subsystem2_test();
    subsystem3_test();
    subsystem4_test();
    subsystem5_test();
    integration_test();
    tm_printf((UB *)"[TEST] === test run end ===\n");
#endif
    REQUIRE(subsystem2_start());
    REQUIRE(subsystem3_start());
    REQUIRE(subsystem4_start());
    REQUIRE(subsystem5_start());
    REQUIRE(mission_start());
    REQUIRE(subsystem1_start());
    mission_release();
    tm_printf(
        (UB *)"all subsystems started; unconfirmed settings inhibit mission\n");

    // sleep forever, returning from usermain() would shut down.
    for (;;)
        tk_slp_tsk(TMO_FEVR);
}

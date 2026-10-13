#include "subsystem5.h"
#include "tk/syslib.h"

#define ECHO_PIN 2
#define TRIGGER_PIN 3
#define ECHO_TIMEOUT_US 30000u
#define SAMPLE_DELAY_MS 250u
#define SONAR_TASK_PRIORITY 17

ER subsystem5_init(void)
{
    ER error;

    error = gpio_set_val(TRIGGER_PIN, 0);
    if (error != E_OK)
    {
        return error;
    }

    error = gpio_set_pin(TRIGGER_PIN, GPIO_MODE_OUT);
    if (error != E_OK)
    {
        return error;
    }

    error = gpio_set_pin(ECHO_PIN, GPIO_MODE_IN);
    if (error < E_OK)
    {
        return error;
    }

    return E_OK;
}
/* Trigger and Echo timing use the shared 1 us hardware timebase. */
static ER ultrasonic_trigger(void)
{
    ER error;

    error = gpio_set_val(TRIGGER_PIN, 0);
    if (error < E_OK)
    {
        return error;
    }
    WaitUsec(2);

    error = gpio_set_val(TRIGGER_PIN, 1);
    if (error < E_OK)
    {
        return error;
    }
    WaitUsec(10);

    return gpio_set_val(TRIGGER_PIN, 0);
}

static ER ultrasonic_wait_for_echo(void)
{
    ER error;
    UW wait_start;

    /* A pre-existing HIGH is not a new response to this trigger. */
    if (gpio_get_val(ECHO_PIN) != 0u)
    {
        tm_printf((UB *)"[S5] INVALID: Echo HIGH before trigger\n");
        return E_TMOUT;
    }

    error = ultrasonic_trigger();
    if (error < E_OK)
    {
        return error;
    }

    wait_start = in_w(TIMER_TIMERAWL);
    while (gpio_get_val(ECHO_PIN) == 0u)
    {
        if ((UW)(in_w(TIMER_TIMERAWL) - wait_start) >= ECHO_TIMEOUT_US)
        {
            tm_printf((UB *)"[S5] TIMEOUT: no Echo rise; distance unknown\n");
            return E_TMOUT;
        }
    }

    /* Echo has become HIGH. */
    /* Echo is HIGH: record the start time in microseconds. */
    UW pulse_start = in_w(TIMER_TIMERAWL);

    /* Wait for Echo to become LOW. */
    while (gpio_get_val(ECHO_PIN) == 1)
    {
        if ((UW)(in_w(TIMER_TIMERAWL) - pulse_start) >= ECHO_TIMEOUT_US)
        {
            tm_printf((UB *)"[S5] TIMEOUT: Echo stayed HIGH; distance unknown\n");
            return E_TMOUT;
        }
    }

    /* Calculate how long Echo stayed HIGH. */
    UW pulse_end = in_w(TIMER_TIMERAWL);
    UW duration_us = pulse_end - pulse_start;
    /* Approximate distance in whole centimetres. */
    UW distance_cm = duration_us / 58u;

    if (duration_us >= ECHO_TIMEOUT_US)
    {
        return E_TMOUT;
    }

    tm_printf((UB *)"[S5] Distance: %u cm\n",
              (unsigned int)distance_cm);

    tm_printf((UB *)"[S5] Echo duration: %u us\n",
              (unsigned int)duration_us);

    return E_OK;
}

static void sonar_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    tm_printf((UB *)"[S5] Serial range bench: TRIG=GP3 ECHO=GP2\n");
    for (;;)
    {
        ER error = ultrasonic_wait_for_echo();
        if (error < E_OK && error != E_TMOUT)
        {
            tm_printf((UB *)"[S5] acquisition error: %d\n", (int)error);
        }
        /* Delay after completion also keeps trigger spacing above 60 ms. */
        tk_dly_tsk(SAMPLE_DELAY_MS);
    }
}

ER subsystem5_start(void)
{
    ID task = app_start_task((FP)sonar_task, SONAR_TASK_PRIORITY,
                             TP_PRC2, "sonar_task");
    return task < E_OK ? task : E_OK;
}

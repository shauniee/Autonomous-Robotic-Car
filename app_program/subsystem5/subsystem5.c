#include "subsystem5.h"
#include <stdatomic.h>
#include "../common/drv_gpio_irq.h"
#include "../common/robot_time.h"
#include "../common/robot_config.h"

/* Echo capture state. The handler is the only writer once the task arms the
   measurement with the Echo IRQ disabled, so no field has two writers at once. */
enum { ECHO_IDLE, ECHO_ARMED, ECHO_HIGH, ECHO_DONE };
static _Atomic unsigned echo_state;
static _Atomic uint32_t pulse_us;
static uint32_t rise_us;			/* handler-private */
static rd_word_t front_word;			/* writer: the obstacle task */

uint32_t s5_front_word(void)
{
    return rd_word_read(&front_word);
}

static void echo_edge(unsigned pin, uint32_t edges, uint32_t sampled_us)
{
    (void)pin;
    unsigned state = atomic_load_explicit(&echo_state, memory_order_relaxed);
    if (edges == GPIO_IRQ_EDGE_RISE && state == ECHO_ARMED)
    {
        rise_us = sampled_us;
        atomic_store_explicit(&echo_state, ECHO_HIGH, memory_order_release);
    }
    else if (edges == GPIO_IRQ_EDGE_FALL && state == ECHO_HIGH)
    {
        atomic_store_explicit(&pulse_us, robot_time_elapsed(sampled_us, rise_us), memory_order_relaxed);
        atomic_store_explicit(&echo_state, ECHO_DONE, memory_order_release);
    }
    /* Any other combination (both flags, stray edge) leaves the measurement to time out. */
}

ER subsystem5_init(void)
{
    ER error = gpio_set_val(TRIGGER_PIN, 0);
    if (error >= E_OK)
    {
        error = gpio_set_pin(TRIGGER_PIN, GPIO_MODE_OUT);
    }
    if (error >= E_OK)
    {
        error = gpio_set_pin(ECHO_PIN, GPIO_MODE_IN);
    }
    return error;
}

/* 10 us trigger pulse from the hardware TIMER; the only busy wait in S5. */
static ER trigger(void)
{
    ER error = gpio_set_val(TRIGGER_PIN, 0);
    if (error < E_OK) return error;
    uint32_t t = robot_time_us();
    while (robot_time_elapsed(robot_time_us(), t) < 2u) { }
    error = gpio_set_val(TRIGGER_PIN, 1);
    if (error < E_OK) return error;
    t = robot_time_us();
    while (robot_time_elapsed(robot_time_us(), t) < 10u) { }
    return gpio_set_val(TRIGGER_PIN, 0);
}

static void obstacle_task(INT stacd, void *exinf)
{
    (void)stacd; (void)exinf;
    bool ready = drv_gpio_irq_init() >= E_OK && drv_gpio_irq_claim(ECHO_PIN, echo_edge);
    if (!ready)
    {
        rd_fault_set(RD_OWNER_OBSTACLE, RD_FAULT_NO_ECHO);
        tm_printf((UB *)"[S5] echo dispatcher unavailable; front range unknown\n");
    }
    uint32_t release = robot_time_ms(), misses = 0u;
    for (;;)
    {
        if (ready)
        {
            (void)drv_gpio_irq_enable(ECHO_PIN, false);
            atomic_store_explicit(&echo_state, ECHO_ARMED, memory_order_release);
            (void)drv_gpio_irq_enable(ECHO_PIN, true);
            uint32_t sent_us = robot_time_us();
            if (trigger() >= E_OK)
            {
                /* Wait for the echo without blocking other tasks: poll the state per tick. */
                while (atomic_load_explicit(&echo_state, memory_order_acquire) != ECHO_DONE &&
                       robot_time_elapsed(robot_time_us(), sent_us) < SONAR_TIMEOUT_US)
                {
                    tk_dly_tsk(1);
                }
            }
            (void)drv_gpio_irq_enable(ECHO_PIN, false);
            bool done = atomic_load_explicit(&echo_state, memory_order_acquire) == ECHO_DONE;
            uint32_t range = done ? echo_range_mm(atomic_load_explicit(&pulse_us, memory_order_relaxed))
                                  : ECHO_RANGE_INVALID;
            atomic_store_explicit(&echo_state, ECHO_IDLE, memory_order_release);
            rd_word_publish(&front_word, echo_pack(range, robot_time_ms()));
            if (echo_note(&misses, range != ECHO_RANGE_INVALID))
            {
                rd_fault_set(RD_OWNER_OBSTACLE, RD_FAULT_NO_ECHO);
            }
            else
            {
                rd_fault_clear(RD_OWNER_OBSTACLE, RD_FAULT_NO_ECHO);
            }
        }
        tk_dly_tsk(robot_time_next_delay(&release, SONAR_REPEAT_MS, robot_time_ms()));
    }
}

ER subsystem5_start(void)
{
    /* Boot never waits for an echo: ranging runs in its own periodic task. */
    ID tid = app_start_task((FP)obstacle_task, S5_PRI_OBSTACLE, TP_PRC2, "Obstacle");
    return tid < E_OK ? tid : E_OK;
}

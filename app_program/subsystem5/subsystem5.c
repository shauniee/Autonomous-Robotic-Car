#include "subsystem5.h"
#include "tk/syslib.h"


#define ECHO_PIN 2
#define TRIGGER_PIN 3

ER subsystem5_init(void) 
{ 
    ER error;

    error = gpio_set_val(TRIGGER_PIN, 0);
    if (error != E_OK) {
        return error;
    }

    error = gpio_set_pin(TRIGGER_PIN, GPIO_MODE_OUT);
    if (error != E_OK) {
        return error;
    }

    error = gpio_set_pin(ECHO_PIN, GPIO_MODE_IN);
    if (error < E_OK) {
        return error;
    }

    return E_OK; }
/*ULTRASONIC FUNCTIONS!!!!! :)*/
static ER ultrasonic_trigger(void)
{ 
    ER error;

    error = gpio_set_val(TRIGGER_PIN, 0);
    if (error < E_OK) {
        return error;
    }
    WaitUsec(2);

    error = gpio_set_val(TRIGGER_PIN, 1);
    if (error < E_OK) {
        return error;
    }
    WaitUsec(10);

    return gpio_set_val(TRIGGER_PIN, 0);

    return E_OK; }

static ER ultrasonic_wait_for_echo(void)
{ 
    ER error;
    SYSTIM start; //record the time
    SYSTIM now;

    /* Record when we started waiting. */
    error = tk_get_otm(&start);
    if (error < E_OK) {
        return error;
    }

    error = ultrasonic_trigger();
    if (error < E_OK) {
        return error;
    }

    /* Keep checking while Echo is LOW. */
    while (gpio_get_val(ECHO_PIN) == 0) {
        error = tk_get_otm(&now);
        if (error < E_OK) {
            return error;
        }

        /* Stop waiting after 30 milliseconds. */
        if ((UW)(now.lo - start.lo) >= 30u) {
            return E_TMOUT;
        }
    }

    /* Echo has become HIGH. */
    return E_OK;}



ER subsystem5_start(void) 
{ 
    return ultrasonic_trigger(); 
    return ultrasonic_wait_for_echo();
    return E_OK; }


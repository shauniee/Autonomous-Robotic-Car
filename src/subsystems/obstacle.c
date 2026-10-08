#include "robot/subsystems.h"

/* Only Pico builds need the hardware library; desktop checks skip it. */
#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
#include "pico/stdlib.h"

#define ECHO_PIN 2
#define TRIGGER_PIN 3
#endif

void obstacle_init(void) {
#if defined(PICO_ON_DEVICE) && PICO_ON_DEVICE
    /* Trigger: the Pico sends a signal to the ultrasonic sensor. */
    gpio_init(TRIGGER_PIN);
    gpio_put(TRIGGER_PIN, 0); /* Start LOW: no measurement requested. */
    gpio_set_dir(TRIGGER_PIN, GPIO_OUT);

    /* Echo: the Pico receives a signal from the ultrasonic sensor. */
    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);


#endif
}

ObstacleReading obstacle_update(uint32_t now_ms) {
    (void)now_ms; /* Time is not used in this first step. */

    /* No measurement yet. Zero initialization makes status.valid false. */
    ObstacleReading reading = {0};
    return reading;
}

/* Mission state on the onboard WS2812 RGB LEDs (GP18). Mission task only. */
#ifndef STATUS_LED_H
#define STATUS_LED_H

#include "../app.h"
#include "mission_logic.h"

/* Claims PIO0 state machine 0 and GP18; LEDs start dark. */
ER status_led_init(void);

/* Sends one static colour to every LED. False when not initialized, the
   state is out of range or the previous frame is still queued: retry on the
   next release. */
bool status_led_show(MissionState_t state);

#endif /* STATUS_LED_H */

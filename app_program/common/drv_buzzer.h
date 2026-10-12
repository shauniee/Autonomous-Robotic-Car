#ifndef DRV_BUZZER_H
#define DRV_BUZZER_H

#include <stdbool.h>

bool drv_buzzer_init(void);

/* true: produce tone; false: silence. */
bool drv_buzzer_set_tone(bool enabled);

#endif
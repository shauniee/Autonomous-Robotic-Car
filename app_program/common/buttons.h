#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    BUTTON_EVENT_NONE,
    BUTTON_EVENT_PRESSED,
    BUTTON_EVENT_RELEASED,
    BUTTON_EVENT_IGNORED_PRESS
} ButtonEvent_t;

typedef struct
{
    uint32_t raw_changed_ms;
    bool last_raw_down : 1;
    bool debounced_down : 1;
    bool press_allowed : 1;
} Button_t;

typedef struct
{
    bool gp20_down : 1;
    bool gp21_down : 1;
} ButtonInputs_t;

bool drv_buttons_init(void);
ButtonInputs_t drv_buttons_read(void);

void button_init(Button_t *button, uint32_t now);
void button_disarm(Button_t *button);

ButtonEvent_t button_update(
    Button_t *button,
    bool down,
    uint32_t now);

#endif
#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    BUTTON_EVENT_NONE,
    BUTTON_EVENT_PRESSED,
    BUTTON_EVENT_RELEASED,
    BUTTON_EVENT_IGNORED_PRESS
} ButtonEvent;

typedef struct
{
    bool last_raw_down;
    bool debounced_down;
    bool press_allowed;
    uint32_t raw_changed_ms;
} Button;

void button_init(Button *button, uint32_t now);
void button_disarm(Button *button);

ButtonEvent button_update(
    Button *button,
    bool down,
    uint32_t now);

#endif
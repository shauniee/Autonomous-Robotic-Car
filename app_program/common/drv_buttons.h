#ifndef DRV_BUTTONS_H
#define DRV_BUTTONS_H

#include <stdbool.h>

typedef struct
{
    bool gp20_down;
    bool gp21_down;
} ButtonInputs;

bool drv_buttons_init(void);
ButtonInputs drv_buttons_read(void);

#endif
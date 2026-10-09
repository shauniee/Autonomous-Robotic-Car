#ifndef ROBOT_CONTROL_TASK_H
#define ROBOT_CONTROL_TASK_H

#include "app.h"

#define ROBOT_CONTROL_PRIORITY 11
ER robot_control_init(void);
ER robot_control_start(void);
#if APP_TEST
void robot_control_test(void);
#endif

#endif

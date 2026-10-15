#ifndef MISSION_H
#define MISSION_H
#include "../app.h"
#include "mission_status.h"
ER mission_init(void);
ER mission_start(void);
void mission_release(void);
#endif

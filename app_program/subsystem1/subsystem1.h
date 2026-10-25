/* Shaun: bounded diagnostic snapshot; kernel service owns every network call. */
#ifndef SUBSYSTEM1_H
#define SUBSYSTEM1_H
#include "../app.h"
#include "telemetry.h"
ER subsystem1_init(void);
ER subsystem1_start(void);
#endif

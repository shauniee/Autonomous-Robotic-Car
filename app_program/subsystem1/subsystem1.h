/* Shaun: bounded diagnostic snapshot; kernel service owns every network call. */
#ifndef SUBSYSTEM1_H
#define SUBSYSTEM1_H
#ifdef BENCH_KERNEL_HOST_TEST
#include "kernel_io.h"
#else
#include "../app.h"
#endif
#include "telemetry.h"
ER subsystem1_init(void);
ER subsystem1_start(void);
#endif

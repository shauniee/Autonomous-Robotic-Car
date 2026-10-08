/* Nicholas: line/barcode ownership; current build implements the IR bench. */
#ifndef SUBSYSTEM3_H
#define SUBSYSTEM3_H
#ifdef BENCH_KERNEL_HOST_TEST
#include "kernel_io.h"
#else
#include "../app.h"
#endif
#define S3_PRI_BENCH 12
ER subsystem3_init(void);
ER subsystem3_start(void);
#endif

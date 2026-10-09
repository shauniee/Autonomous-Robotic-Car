#ifndef TASK_IR_H
#define TASK_IR_H

#include "subsystem3.h"

/* Private S3 task entry; owns ADC, calibration inputs and feedback. */
void task_ir_task(INT stacd, void *exinf);

#endif

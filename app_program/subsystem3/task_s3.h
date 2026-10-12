#ifndef TASK_S3_H
#define TASK_S3_H

#include "subsystem3.h"

/* Private S3 task entry; owns ADC, calibration inputs and request_buzzer_chirps. */
void task_s_task(INT stacd, void *exinf);

#endif

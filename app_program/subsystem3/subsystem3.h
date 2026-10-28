#ifndef SUBSYSTEM3_H
#define SUBSYSTEM3_H

#include "../app.h"
#include "ir_tracking.h"
#include "barcode.h"
#include "../common/robot_config.h"


ER subsystem3_init(void);
ER subsystem3_start(void);
/* 
 * Coherent copy; waits at most 2 ms for the publisher. False means null,
 * uninitialized, or lock/unlock error;
 * Ignore the destination on failure; normalized zero is not validity.
 * This API is for task context, never an ISR.
*/
bool s3_ir_copy(S3IrSnapshot *snapshot);

#endif

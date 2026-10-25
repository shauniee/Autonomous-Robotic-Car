#ifndef SUBSYSTEM3_H
#define SUBSYSTEM3_H

#include "../app.h"
#include "ir_snapshot.h"
#include "barcode.h"
#include "../common/robot_config.h"


ER subsystem3_init(void);
ER subsystem3_start(void);
/* 
 * Nonblocking coherent copy. False means null, uninitialized, or busy/error;
 * Ignore the destination on failure; normalized zero is not validity.
 * This API is for task context, never an ISR.
*/
bool s3_ir_copy(S3IrSnapshot *snapshot);

#endif

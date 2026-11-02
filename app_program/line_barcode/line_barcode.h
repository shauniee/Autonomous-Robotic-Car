#ifndef LINE_BARCODE_H
#define LINE_BARCODE_H

#include "../app.h"
#include "line.h"
#include "barcode.h"
#include "../common/robot_config.h"


ER line_barcode_init(void);
void line_barcode_step(uint32_t now_ms);
/* 
 * Coherent copy; waits at most 2 ms for the publisher. False means null,
 * uninitialized, or lock/unlock error;
 * Ignore the destination on failure; normalized zero is not validity.
 * This API is for task context, never an ISR.
*/
bool line_ir_copy(LineIrSnapshot *snapshot);
bool line_ir_try_copy(LineIrSnapshot *snapshot);

#endif

#ifndef DRV_IR_H
#define DRV_IR_H
#include "ir_tracking.h"
bool drv_ir_init(void);
bool drv_ir_read(IrRaw *raw);
#endif

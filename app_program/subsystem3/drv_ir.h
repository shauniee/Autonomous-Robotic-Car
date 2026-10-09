#ifndef DRV_IR_H
#define DRV_IR_H

#include "ir_tracking.h"

/* Called only by S3; shared timebase must already be initialized. */
bool drv_ir_init(void);
/* False leaves *raw unchanged. No connectivity inference from ADC counts. */
bool drv_ir_read(IrRaw *raw);

#endif

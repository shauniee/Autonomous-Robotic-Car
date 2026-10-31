#ifndef DRV_IR_TRACKING_H
#define DRV_IR_TRACKING_H

#include "ir_tracking.h"

/* Called only by S3; shared timebase must already be initialized. */
bool drv_ir_tracking_init(void);
/* False leaves *raw unchanged. No connectivity inference from digital levels. */
bool drv_ir_tracking_read(IrRaw *raw);

#endif

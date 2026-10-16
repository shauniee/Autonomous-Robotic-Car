#ifndef DRV_IR_TRACKING_AVERAGE_H
#define DRV_IR_TRACKING_AVERAGE_H

#include <stdint.h>

#define DRV_IR_TRACKING_AVERAGE_COUNT 8u

/* Keep the driver and its deterministic fixture on the same fixed sample set. */
static inline uint16_t
drv_ir_tracking_average(const uint16_t samples[DRV_IR_TRACKING_AVERAGE_COUNT])
{
    uint32_t sum = 0u;
    for (unsigned i = 0u; i < DRV_IR_TRACKING_AVERAGE_COUNT; ++i)
    {
        sum += samples[i];
    }
    return (uint16_t)(sum / DRV_IR_TRACKING_AVERAGE_COUNT);
}

#endif

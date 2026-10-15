#ifndef DRV_ENCODER_H
#define DRV_ENCODER_H
#include <stdbool.h>
#include <stdint.h>
typedef struct
{
    uint32_t left_count, right_count, sampled_us, epoch, faults;
    bool valid;
} EncoderCapture;
bool drv_encoder_init(void);
/* Direct bounded capture, also valid from core-1 ISR/IRQ-masked callback. */
bool drv_encoder_copy(EncoderCapture *out);
int drv_encoder_transition(unsigned previous, unsigned current);
#endif

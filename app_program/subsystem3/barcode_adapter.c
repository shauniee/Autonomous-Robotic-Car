#include "barcode_adapter.h"
#include "drv_barcode.h"
#include "../subsystem2/drv_encoder.h"
#include "../common/drv_gpio_irq.h"
static bool read_position(void *user, bool *level, barcode_encoder_t *out,
                          uint32_t *sampled_us)
{
    (void)user;
    EncoderCapture capture;
    if (!drv_encoder_copy(&capture))
    {
        return false;
    }
    *sampled_us = capture.sampled_us;
    *level = gpio_get_val(16u) != 0;
    *out =
        (barcode_encoder_t){capture.left_count, capture.right_count,
                            capture.sampled_us, capture.epoch, capture.valid};
    return true;
}
static void barcode_edge(unsigned pin, uint32_t edges, uint32_t now)
{
    (void)pin;
    EncoderCapture capture;
    if (!drv_encoder_copy(&capture))
    {
        drv_barcode_edge_isr(edges, now, NULL);
        return;
    }
    barcode_encoder_t encoder = {capture.left_count, capture.right_count, now,
                                 capture.epoch, capture.valid};
    drv_barcode_edge_isr(edges, now, &encoder);
}
static bool set_enabled(void *user, bool enabled)
{
    (void)user;
    return drv_gpio_irq_claim(16u, barcode_edge) &&
           drv_gpio_irq_enable(16u, enabled);
}
const barcode_provider_t *barcode_adapter_provider(void)
{
    static const barcode_provider_t provider = {read_position, set_enabled,
                                                NULL};
    return &provider;
}

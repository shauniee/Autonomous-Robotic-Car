/* Use the port guard when mixing ISO C headers with tk/syslib.h. */
#define PROHIBIT_DEF_SIZE_T
#include "line_barcode.h"
#include <stddef.h>
#ifndef HOST_TEST
#include "../app.h"
#endif

/* ---- IR tracking logic ---- */
IrReading_t ir_tracking_reading(IrRaw_t raw)
{
    IrReading_t reading = {0};
    if (raw.left > 1u || raw.right > 1u)
    {
        return reading;
    }
    reading.left_fraction = IR_BLACK_LEVEL == raw.left ? IR_SCALE_ONE : 0u;
    reading.right_fraction = IR_BLACK_LEVEL == raw.right ? IR_SCALE_ONE : 0u;
    reading.error = (int8_t)(reading.right_fraction - reading.left_fraction);
    reading.valid = true;
    return reading;
}

void ir_tracking_init(IrTracker_t *tracker)
{
    if (tracker != NULL)
    {
        *tracker = (IrTracker_t){0};
    }
}

void ir_tracking_sample(IrTracker_t *tracker, IrRaw_t raw, bool acquired,
    uint32_t now)
{
    if (NULL == tracker)
    {
        return;
    }
    IrReading_t reading = ir_tracking_reading(raw);
    reading.valid = acquired && reading.valid;
    tracker->raw = ir_raw_word(raw, reading.valid);
    tracker->have_sample = 1u;
    tracker->sample_ms = now;
    tracker->line = (uint8_t)(!reading.valid ? IR_LINE_UNKNOWN :
        (reading.left_fraction && reading.right_fraction ?
            IR_LINE_BROAD_BLACK : IR_LINE_VISIBLE));
}

IrTrackingOutput_t ir_tracking_output(const IrTracker_t *tracker, uint32_t now)
{
    IrTrackingOutput_t output = {0};
    if (NULL == tracker)
    {
        return output;
    }
    output.sampled_ms = tracker->sample_ms;
    bool usable = tracker->have_sample && ir_raw_word_valid(tracker->raw) &&
        (uint32_t)(now - tracker->sample_ms) <= IR_MAX_GAP_MS;
    output.line = usable ? tracker->line : (uint8_t)IR_LINE_UNKNOWN;
    output.valid = usable && IR_LINE_VISIBLE == output.line;
    output.error = output.valid ?
        ir_tracking_reading(ir_raw_word_raw(tracker->raw)).error : 0;
    return output;
}

void ir_snapshot_refresh(LineIrSnapshot_t *snapshot, uint32_t now)
{
    if (NULL == snapshot)
    {
        return;
    }
    snapshot->sample_fresh = snapshot->have_sample &&
        ir_raw_word_valid(snapshot->raw) &&
        (uint32_t)(now - snapshot->sampled_ms) <= IR_MAX_GAP_MS;
    if (!snapshot->sample_fresh)
    {
        snapshot->normalized_valid = 0u;
        snapshot->guidance_valid = 0u;
        snapshot->left_black = 0u;
        snapshot->right_black = 0u;
        snapshot->line = IR_LINE_UNKNOWN;
    }
}

LineIrSnapshot_t ir_snapshot_build(const IrTracker_t *tracker, uint32_t now)
{
    LineIrSnapshot_t snapshot = {0};
    if (NULL == tracker)
    {
        return snapshot;
    }
    snapshot.sampled_ms = tracker->sample_ms;
    snapshot.raw = ir_raw_word(ir_raw_word_raw(tracker->raw),
        tracker->have_sample && ir_raw_word_valid(tracker->raw));
    snapshot.have_sample = tracker->have_sample;
    IrReading_t reading = ir_tracking_reading(ir_raw_word_raw(tracker->raw));
    snapshot.normalized_valid = ir_raw_word_valid(snapshot.raw);
    if (snapshot.normalized_valid)
    {
        snapshot.left_black = reading.left_fraction != 0u;
        snapshot.right_black = reading.right_fraction != 0u;
    }
    IrTrackingOutput_t output = ir_tracking_output(tracker, now);
    snapshot.guidance_valid = output.valid;
    snapshot.line = (uint8_t)output.line;
    ir_snapshot_refresh(&snapshot, now);
    return snapshot;
}

#if APP_TEST
const char *ir_tracking_line_name(IrLine_t line)
{
    switch (line)
    {
        case IR_LINE_VISIBLE: return "visible";
        case IR_LINE_LOST: return "lost";
        case IR_LINE_BROAD_BLACK: return "broad_black";
        default: return "unknown";
    }
}

#endif

/* ---- IR GPIO driver ---- */
#ifndef HOST_TEST
/* Connect each module's DOUT, with AOUT disconnected. */
static bool configure_input(unsigned pin)
{
    if (gpio_set_pin(pin, GPIO_MODE_IN) < E_OK)
    {
        return false;
    }
    clr_w(GPIO(pin), GPIO_PUE | GPIO_PDE);
    return true;
}

bool drv_ir_tracking_init(void)
{
    return configure_input(IR_LEFT_PIN) && configure_input(IR_RIGHT_PIN);
}

bool drv_ir_tracking_read(IrRaw_t *raw)
{
    if (NULL == raw)
    {
        return false;
    }
    /* One GPIO snapshot keeps both sensor states from the same instant. */
    uint32_t levels = in_w(GPIO_IN);
    raw->left = (uint8_t)((levels >> IR_LEFT_PIN) & 1u);
    raw->right = (uint8_t)((levels >> IR_RIGHT_PIN) & 1u);
    return true;
}
#endif /* !HOST_TEST */

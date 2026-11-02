/* Use the port guard when mixing ISO C headers with tk/syslib.h. */
#define PROHIBIT_DEF_SIZE_T
#include "line.h"
#include <stddef.h>
#ifndef HOST_TEST
#include "../app.h"
#endif

/* ---- IR tracking logic ---- */
IrReading ir_tracking_reading(IrRaw raw)
{
    IrReading reading = {0};
    if (raw.left > 1u || raw.right > 1u)
    {
        return reading;
    }
    reading.left_fraction = raw.left == IR_BLACK_LEVEL ? IR_SCALE_ONE : 0u;
    reading.right_fraction = raw.right == IR_BLACK_LEVEL ? IR_SCALE_ONE : 0u;
    reading.error = (int8_t)(reading.right_fraction - reading.left_fraction);
    reading.valid = true;
    return reading;
}

void ir_tracking_init(IrTracker *tracker)
{
    if (tracker != NULL)
    {
        *tracker = (IrTracker){0};
    }
}

void ir_tracking_sample(IrTracker *tracker, IrRaw raw, bool acquired,
    uint32_t now)
{
    if (tracker == NULL)
    {
        return;
    }
    IrReading reading = ir_tracking_reading(raw);
    reading.valid = acquired && reading.valid;
    tracker->raw = ir_raw_word(raw, reading.valid);
    tracker->have_sample = true;
    tracker->sample_ms = now;
    tracker->reading = reading;
    tracker->line = !reading.valid ? IR_LINE_UNKNOWN :
        (reading.left_fraction && reading.right_fraction ?
            IR_LINE_BROAD_BLACK : IR_LINE_VISIBLE);
}

IrTrackingOutput ir_tracking_output(const IrTracker *tracker, uint32_t now)
{
    IrTrackingOutput output = {0};
    if (tracker == NULL)
    {
        return output;
    }
    output.sampled_ms = tracker->sample_ms;
    bool usable = tracker->have_sample && ir_raw_word_valid(tracker->raw) &&
        tracker->reading.valid &&
        (uint32_t)(now - tracker->sample_ms) <= IR_MAX_GAP_MS;
    output.line = usable ? tracker->line : IR_LINE_UNKNOWN;
    output.valid = usable && output.line == IR_LINE_VISIBLE;
    output.error = output.valid ? tracker->reading.error : 0;
    return output;
}

void ir_snapshot_refresh(LineIrSnapshot *snapshot, uint32_t now)
{
    if (snapshot == NULL)
    {
        return;
    }
    snapshot->sample_age_ms = snapshot->have_sample ?
        (uint32_t)(now - snapshot->sampled_ms) : 0u;
    snapshot->sample_fresh = snapshot->have_sample &&
        ir_raw_word_valid(snapshot->raw) &&
        (snapshot->sample_age_ms <= IR_MAX_GAP_MS);
    if (!snapshot->sample_fresh)
    {
        snapshot->normalized_valid = false;
        snapshot->guidance_valid = false;
        snapshot->left_fraction = 0u;
        snapshot->right_fraction = 0u;
        snapshot->error = 0;
        snapshot->line = IR_LINE_UNKNOWN;
    }
}

LineIrSnapshot ir_snapshot_build(const IrTracker *tracker, uint32_t sequence,
    uint32_t now)
{
    LineIrSnapshot snapshot = {0};
    if (tracker == NULL)
    {
        return snapshot;
    }
    snapshot.sequence = sequence;
    snapshot.sampled_ms = tracker->sample_ms;
    snapshot.raw = ir_raw_word(ir_raw_word_raw(tracker->raw),
        tracker->have_sample && ir_raw_word_valid(tracker->raw));
    snapshot.have_sample = tracker->have_sample;
    IrReading reading = tracker->reading;
    snapshot.normalized_valid = ir_raw_word_valid(snapshot.raw) && reading.valid;
    if (snapshot.normalized_valid)
    {
        snapshot.left_fraction = reading.left_fraction;
        snapshot.right_fraction = reading.right_fraction;
        snapshot.error = reading.error;
    }
    IrTrackingOutput output = ir_tracking_output(tracker, now);
    snapshot.guidance_valid = output.valid;
    snapshot.line = output.line;
    ir_snapshot_refresh(&snapshot, now);
    return snapshot;
}

#if APP_TEST
const char *ir_tracking_line_name(IrLine line)
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

bool drv_ir_tracking_read(IrRaw *raw)
{
    if (raw == NULL)
    {
        return false;
    }
    /* One GPIO snapshot keeps both sensor states from the same instant. */
    uint32_t levels = in_w(GPIO_IN);
    raw->left = (uint16_t)((levels >> IR_LEFT_PIN) & 1u);
    raw->right = (uint16_t)((levels >> IR_RIGHT_PIN) & 1u);
    return true;
}
#endif /* !HOST_TEST */

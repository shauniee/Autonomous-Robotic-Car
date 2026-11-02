#include <assert.h>
#include <stdio.h>
#include "../../app_program/line_barcode/line_barcode.h"

int main(void)
{
    IrTracker tracker;
    ir_tracking_init(&tracker);
    /* Regression: LOW/LOW white must not become a junction. */
    ir_tracking_sample(&tracker, (IrRaw){0, 0}, true, 80u);
    IrTrackingOutput white = ir_tracking_output(&tracker, 80u);
    assert(white.line == IR_LINE_VISIBLE && white.valid && white.error == 0);
    ir_tracking_sample(&tracker, (IrRaw){1, 1}, true, 100u);
    assert(ir_tracking_output(&tracker, 100u).line == IR_LINE_BROAD_BLACK);
    for (unsigned left = 0; left <= 1; ++left)
    {
        for (unsigned right = 0; right <= 1; ++right)
        {
            ir_tracking_sample(&tracker, (IrRaw){left, right}, true, 100u);
            LineIrSnapshot s = ir_snapshot_build(&tracker, 1u, 100u);
            bool lb = left == IR_BLACK_LEVEL;
            bool rb = right == IR_BLACK_LEVEL;
            assert(s.sample_fresh);
            assert(s.line == (lb && rb ? IR_LINE_BROAD_BLACK : IR_LINE_VISIBLE));
            assert(s.guidance_valid == !(lb && rb));
            assert(s.error == (lb && !rb ? -127 : (rb && !lb ? 127 : 0)));
            ir_snapshot_refresh(&s, 100u + IR_MAX_GAP_MS + 1u);
            assert(!s.guidance_valid && !s.sample_fresh && s.line == IR_LINE_UNKNOWN);
        }
    }
    ir_tracking_sample(&tracker, (IrRaw){2, 0}, true, 200u);
    assert(!ir_tracking_output(&tracker, 200u).valid);
    ir_tracking_sample(&tracker, (IrRaw){0, 1}, false, 220u);
    assert(ir_tracking_output(&tracker, 220u).line == IR_LINE_UNKNOWN);
    puts("digital IR: PASS (four states, stale, invalid, failed acquisition)");
    return 0;
}

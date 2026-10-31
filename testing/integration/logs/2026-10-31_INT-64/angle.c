#include <assert.h>
#include <stdio.h>
#include "app_program/mission/recovery_control.h"
#include "app_program/subsystem2/encoder_math.h"

int main(void)
{
    recovery_control_t control;
    recovery_input_t in = {.now_ms = 100u, .sampled_ms = 100u,
        .functional = true, .stationary = true};
    recovery_output_t out;
    recovery_control_begin(&control, &in);
    recovery_control_step(&control, &in, &out);
    assert(out.turn_degrees == 70 && !control.right);
    in.stationary = false;
    in.count[0] = enc_advance(0u, -20);
    in.count[1] = enc_advance(0u, 20);
    in.sampled_ms = in.now_ms += IR_SAMPLE_MS;
    recovery_control_step(&control, &in, &out);
    assert(out.turn && !control.right);
    printf("left=-20 right=20: estimated %.2f deg; held turn=%d; no reversal\n",
        out.heading_cdeg / 100.0, out.turn_degrees);
    in.count[0] = enc_advance(0u, -130);
    in.count[1] = enc_advance(0u, 130);
    in.sampled_ms = in.now_ms += IR_SAMPLE_MS;
    recovery_control_step(&control, &in, &out);
    assert(!out.turn && control.right);
    printf("left=-130 right=130: estimated %.2f deg; brake then reverse\n",
        out.heading_cdeg / 100.0);
    in.stationary = true;
    recovery_control_step(&control, &in, &out);
    assert(out.new_turn && out.turn_degrees == -135);
    printf("next relative target=%d deg (toward -70 absolute)\n", out.turn_degrees);
    puts("INT-64 estimated-angle assertions PASS; no physical calibration claim");
    return 0;
}

/* Defect reproduction: flat chassis, synthetic fore/aft acceleration. */
#include <assert.h>
#include <stdio.h>
#include "app_program/subsystem4/terrain_algo.h"

int main(void)
{
    TerrainContext context;
    terrain_algo_init(&context);
    Gy511RawAxes flat = {.z = 500};
    for (unsigned i = 0; i < 100; ++i)
        terrain_algo_calibrate_sample(&context, &flat);
    Gy511RawAxes accelerating = {.y = 500, .z = 500};
    Gy511RawAxes braking = {.y = -500, .z = 500};
    for (unsigned i = 0; i < 50; ++i)
        terrain_algo_process(&context, &accelerating, 2.0f);
    for (unsigned i = 0; i < 50; ++i)
        terrain_algo_process(&context, &braking, 2.0f);
    for (unsigned i = 0; i < 100; ++i)
        terrain_algo_process(&context, &flat, 2.0f);
    printf("Flat-chassis acceleration trace: hump_count=%u maximum_code=%u valid=%u\n",
        context.hump_count, context.max_hump_code, context.height_valid);
    assert(context.hump_count > 0 && context.height_valid);
}

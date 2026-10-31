#include <assert.h>
#include <stdio.h>
#include "app_program/subsystem2/encoder_math.h"

int main(void)
{
    uint32_t before = enc_mean(1000u, 33767u);
    uint32_t after = enc_mean(1001u, 33769u);
    int32_t moved = enc_travel_signed(after, before);
    printf("Both wheels moved forward (+1, +2); enc_mean delta=%d counts\n",
        (int)moved);
    assert(moved == -32766);
    puts("Defect reproduced: forward wheel increments became a large reverse delta.");
    return 0;
}

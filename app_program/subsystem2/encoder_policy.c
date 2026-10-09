#include "encoder.h"

int s2_encoder_transition(unsigned previous, unsigned current)
{
    static const int table[16] = {0, -1, 1, 2, 1, 0, 2, -1,
        -1, 2, 0, 1, 2, 1, -1, 0};
    if ((previous > 3u) || (current > 3u))
    {
        return 2;
    }
    return table[previous * 4u + current];
}

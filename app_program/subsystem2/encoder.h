#ifndef S2_ENCODER_H
#define S2_ENCODER_H
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint32_t left_count, right_count, errors, sequence, captured_us;
    bool valid;
} S2EncoderSnapshot;
/* Pure quadrature table; +1/-1 valid edge, 0 unchanged, 2 lost transition. */
int s2_encoder_transition(unsigned previous, unsigned current);
bool s2_encoder_init(void);
bool s2_encoder_copy(S2EncoderSnapshot *out);
void s2_encoder_enable_context(void);
#endif

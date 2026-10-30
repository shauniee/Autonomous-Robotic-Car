#include "obstacle_profile.h"

bool s5_scan_state_pack(s5_scan_state_t state, uint32_t *word)
{
    if ((unsigned)state >= S5_SCAN_STATE_COUNT || word == 0)
    {
        return false;
    }
    *word = (uint32_t)state;
    return true;
}

s5_scan_state_t s5_scan_state_unpack(uint32_t code)
{
    return code < S5_SCAN_STATE_COUNT ? (s5_scan_state_t)code : S5_SCAN_FAILED;
}

s5_scan_state_t s5_scan_guard(bool cancel, bool expired, bool stationary)
{
    return cancel ? S5_SCAN_CANCELLED : ((expired || !stationary) ? S5_SCAN_FAILED : S5_SCAN_COARSE);
}

/* Host test for app_program/subsystem5/obstacle_profile.[ch] (issue #49). */
#include <assert.h>
#include <stdio.h>
#include "../../app_program/subsystem5/obstacle_profile.h"

int main(void)
{
    /* Scan state fits 3 bits; an invalid state is rejected / decoded as FAILED. */
    uint32_t w = 99u;
    for (unsigned s = 0; s < S5_SCAN_STATE_COUNT; ++s) {
        assert(s5_scan_state_pack((s5_scan_state_t)s, &w) && w == s && w <= 7u);
        assert(s5_scan_state_unpack(w) == (s5_scan_state_t)s);
        for (uint8_t id = 0u; id < 16u; ++id)
        {
            /* Use S5's encoding and Mission's reader, including id zero/max. */
            uint32_t scan = w | s5_scan_id_pack(id);
            assert(s5_scan_id(scan) == id);
            assert(s5_scan_state_unpack(scan & 7u) == (s5_scan_state_t)s);
            assert(s5_scan_id(scan | ~0x7Fu) == id);
        }
    }
    w = 99u;
    assert(!s5_scan_state_pack((s5_scan_state_t)S5_SCAN_STATE_COUNT, &w) && w == 99u);
    assert(s5_scan_state_unpack(8u) == S5_SCAN_FAILED && s5_scan_state_unpack(0xFFFFFFFFu) == S5_SCAN_FAILED);

    /* Scan guard: cancel wins, then expiry or motion fails the scan. */
    assert(s5_scan_guard(true, true, false) == S5_SCAN_CANCELLED);
    assert(s5_scan_guard(false, true, true) == S5_SCAN_FAILED);
    assert(s5_scan_guard(false, false, false) == S5_SCAN_FAILED);
    assert(s5_scan_guard(false, false, true) == S5_SCAN_COARSE);

    /* Minimum-bit packing retains the complete signed angle and 16-bit
       measurement. Both structures are checked against Cortex-M0+ layout. */
    assert(sizeof(ObstacleRay) == 4u && sizeof(ObstacleProfile) == 14u);
    for (int angle = -SERVO_MAX_DEG; angle <= SERVO_MAX_DEG; ++angle) {
        ObstacleRay roundtrip = s5_ray_make((int16_t)angle, 65535u, true);
        assert(roundtrip.valid && roundtrip.angle_deg == angle && roundtrip.range_mm == 65535u);
        uint32_t packed = s5_side_watch_pack(true, (int16_t)angle);
        assert((packed & 1u) != 0u && (packed & ~0x1FFu) == 0u);
        assert(s5_side_watch_angle(packed) == angle);
    }
    ObstacleRay invalid = s5_ray_make((int16_t)(SERVO_MAX_DEG + 1), 42u, true);
    assert(!invalid.valid && invalid.range_mm == 0u);
    assert(s5_side_watch_angle(s5_side_watch_pack(false, -90)) == -SERVO_MAX_DEG);
    assert((s5_side_watch_pack(false, 0) & 1u) == 0u);

    puts("obstacle_profile: PASS");
    return 0;
}

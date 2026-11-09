/* Host test for app_program/obstacle/obstacle_profile.[ch] (issue #49). */
#include <assert.h>
#include <stdio.h>
#include "../../app_program/obstacle/obstacle.h"

int main(void)
{
    /* Scan state fits 3 bits; an invalid state is rejected / decoded as FAILED. */
    uint32_t w = 99u;
    for (unsigned s = 0; s < OBSTACLE_SCAN_STATE_COUNT; ++s) {
        assert(obstacle_scan_state_pack((obstacle_scan_state_t)s, &w) && w == s && w <= 7u);
        assert(obstacle_scan_state_unpack(w) == (obstacle_scan_state_t)s);
        for (uint8_t id = 0u; id < 16u; ++id)
        {
            /* Use Obstacle's encoding and Mission's reader, including id zero/max. */
            uint32_t scan = w | obstacle_scan_id_pack(id);
            assert(obstacle_scan_id(scan) == id);
            assert(obstacle_scan_state_unpack(scan & 7u) == (obstacle_scan_state_t)s);
            assert(obstacle_scan_id(scan | ~0x7Fu) == id);
        }
    }
    w = 99u;
    assert(!obstacle_scan_state_pack((obstacle_scan_state_t)OBSTACLE_SCAN_STATE_COUNT, &w) && w == 99u);
    assert(obstacle_scan_state_unpack(8u) == OBSTACLE_SCAN_FAILED && obstacle_scan_state_unpack(0xFFFFFFFFu) == OBSTACLE_SCAN_FAILED);

    /* Scan guard: cancel wins, then expiry or motion fails the scan. */
    assert(obstacle_scan_guard(true, true, false) == OBSTACLE_SCAN_CANCELLED);
    assert(obstacle_scan_guard(false, true, true) == OBSTACLE_SCAN_FAILED);
    assert(obstacle_scan_guard(false, false, false) == OBSTACLE_SCAN_FAILED);
    assert(obstacle_scan_guard(false, false, true) == OBSTACLE_SCAN_COARSE);

    /* Minimum-bit packing retains the complete signed angle and 13-bit
       measurement. Both structures are checked against Cortex-M0+ layout. */
    assert(sizeof(ObstacleRay) == 3u && sizeof(ObstacleProfile) == 9u);
    assert(obstacle_ray_make(0, 65535u, true).range_mm == ECHO_RANGE_INVALID);	/* never wraps */
    for (int angle = -SERVO_MAX_DEG; angle <= SERVO_MAX_DEG; ++angle) {
        ObstacleRay roundtrip = obstacle_ray_make((int16_t)angle, 8190u, true);
        assert(roundtrip.valid && roundtrip.angle_deg == angle && roundtrip.range_mm == 8190u);
        uint32_t packed = obstacle_side_watch_pack(true, (int16_t)angle);
        assert((packed & 1u) != 0u && (packed & ~0x1FFu) == 0u);
        assert(obstacle_side_watch_angle(packed) == angle);
    }
    ObstacleRay invalid = obstacle_ray_make((int16_t)(SERVO_MAX_DEG + 1), 42u, true);
    assert(!invalid.valid && invalid.range_mm == 0u);
    assert(obstacle_side_watch_angle(obstacle_side_watch_pack(false, -90)) == -SERVO_LOOK_DEG);
    assert((obstacle_side_watch_pack(false, 0) & 1u) == 0u);

    puts("obstacle_profile: PASS");
    return 0;
}

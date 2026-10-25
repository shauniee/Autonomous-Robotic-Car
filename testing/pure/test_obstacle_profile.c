/* Host test for app_program/subsystem5/obstacle_profile.[ch] (issue #49). */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../../app_program/subsystem5/obstacle_profile.h"

int main(void)
{
    /* Obstacle ahead: nearest ray is straight ahead; clearances on both sides. */
    ObstacleRay rays[5] = {{-60, 1500u, true}, {-30, 700u, true}, {0, 400u, true}, {30, 700u, true}, {60, 1500u, true}};
    ObstacleProfile p = {0};
    assert(s5_profile_build(rays, 5u, &p) && p.valid);
    assert(p.closest_angle_deg == 0 && p.closest_mm == 400u && p.obstacle_forward_mm == 400 && p.obstacle_left_mm == 0);
    assert(p.width_mm > 0u && p.left_clearance_mm > 0u && p.right_clearance_mm > 0u);

    /* Obstacle to the left: more clearance to the right than a centred one has. */
    ObstacleRay left[5] = {{-60, 900u, true}, {-30, 800u, true}, {0, 600u, true}, {30, 500u, true}, {60, 450u, true}};
    ObstacleProfile q = {0};
    assert(s5_profile_build(left, 5u, &q));
    assert(q.closest_angle_deg == 60 && q.obstacle_left_mm > 0);

    /* Rejections leave the destination unchanged. */
    ObstacleProfile keep = p;
    assert(!s5_profile_build(rays, 4u, &p) && p.closest_mm == keep.closest_mm);		/* too few rays */
    assert(!s5_profile_build(rays, S5_SCAN_CAPACITY + 1u, &p));				/* too many */
    ObstacleRay bad[5]; for (int i = 0; i < 5; ++i) bad[i] = rays[i];
    bad[2].valid = false;                       assert(!s5_profile_build(bad, 5u, &p));	/* invalid ray */
    bad[2] = rays[2]; bad[2].range_mm = 0u;     assert(!s5_profile_build(bad, 5u, &p));	/* zero range */
    bad[2] = rays[2]; bad[2].angle_deg = SERVO_MAX_DEG + 1; assert(!s5_profile_build(bad, 5u, &p));	/* beyond sweep */
    assert(!s5_profile_build(0, 5u, &p) && !s5_profile_build(rays, 5u, 0));

    /* Fine angles surround the nearest coarse ray, skip measured and out-of-sweep angles. */
    int16_t fine[4];
    unsigned n = s5_fine_angles(rays, 5u, fine, 4u);
    assert(n == 4u && fine[0] == -S5_FINE_STEP_DEG && fine[1] == S5_FINE_STEP_DEG);
    n = s5_fine_angles(left, 5u, fine, 4u);						/* nearest at +60: +70 is out of sweep */
    for (unsigned i = 0; i < n; ++i) assert(fine[i] >= -SERVO_MAX_DEG && fine[i] <= SERVO_MAX_DEG);
    assert(s5_fine_angles(rays, 5u, fine, 0u) == 0u && s5_fine_angles(0, 5u, fine, 4u) == 0u);
    ObstacleRay none[2] = {{0, 100u, false}, {30, 100u, false}};
    assert(s5_fine_angles(none, 2u, fine, 4u) == 0u);

    /* Scan state fits 3 bits; an invalid state is rejected / decoded as FAILED. */
    uint32_t w = 99u;
    for (unsigned s = 0; s < S5_SCAN_STATE_COUNT; ++s) {
        assert(s5_scan_state_pack((s5_scan_state_t)s, &w) && w == s && w <= 7u);
        assert(s5_scan_state_unpack(w) == (s5_scan_state_t)s);
    }
    w = 99u;
    assert(!s5_scan_state_pack((s5_scan_state_t)S5_SCAN_STATE_COUNT, &w) && w == 99u);
    assert(s5_scan_state_unpack(8u) == S5_SCAN_FAILED && s5_scan_state_unpack(0xFFFFFFFFu) == S5_SCAN_FAILED);

    /* Scan guard: cancel wins, then expiry or motion fails the scan. */
    assert(s5_scan_guard(true, true, false) == S5_SCAN_CANCELLED);
    assert(s5_scan_guard(false, true, true) == S5_SCAN_FAILED);
    assert(s5_scan_guard(false, false, false) == S5_SCAN_FAILED);
    assert(s5_scan_guard(false, false, true) == S5_SCAN_COARSE);

    puts("obstacle_profile: PASS");
    return 0;
}

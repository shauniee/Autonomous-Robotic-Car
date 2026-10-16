#include "../../app_program/subsystem5/subsystem5_internal.h"
#include "../../app_program/test_report.h"
#include "../fixture_config.h"
#include <math.h>
#if APP_TEST
void subsystem5_test(void)
{
    TEST_INIT();
    ObstacleSnapshot s = {.ray_count = 5u};
    for (unsigned i = 0u; i < 5u; ++i)
    {
        s.rays[i] = (ObstacleRay){(float)((int)i - 2) * 30.0f,
                                  i == 2u ? 0.2f : 1.0f, 100u, true};
    }
    RobotConfig cfg = fixture_config();
    cfg.sonar_forward_m = 0;
    bool valid = s5_profile_build_config(&s, &cfg) && s.closest_m == 0.2f;
    s.rays[0].valid = false;
    TEST_CHECK(
        "S5-U04", valid && !s5_profile_build_config(&s, &cfg),
        "valid profile nearest point; missing Echo cannot grant clearance");
    TEST_CHECK("S5-U05",
               s5_scan_guard(true, false, true) == SCAN_CANCELLED &&
                   s5_scan_guard(true, true, false) == SCAN_CANCELLED &&
                   s5_scan_guard(false, true, true) == SCAN_FAILED &&
                   s5_scan_guard(false, false, false) == SCAN_FAILED &&
                   s5_scan_guard(false, false, true) == SCAN_RUNNING,
               "cancel dominates timeout/movement including center restore");
    s.rays[0].valid = true;
    cfg.sonar_forward_m = 0.02f;
    TEST_CHECK("S5-U06",
               s5_profile_build_config(&s, &cfg) && s.closest_m > 0.21f &&
                   s.obstacle_forward_m > 0.21f,
               "profile geometry includes measured sensor mounting offset");
    ObstacleSnapshot malformed = {.ray_count = 4u};
    bool profile_rejects = !s5_profile_build_config(NULL, &cfg) &&
                           !s5_profile_build_config(&malformed, &cfg);
    malformed.ray_count = S5_SCAN_CAPACITY + 1u;
    profile_rejects = !s5_profile_build_config(&malformed, &cfg) && profile_rejects;
    malformed.ray_count = 5u;
    for (unsigned i = 0u; i < 5u; ++i)
        malformed.rays[i] = (ObstacleRay){.angle_deg = (float)((int)i - 2) * 30.0f,
                                          .range_m = 1.0f, .valid = true};
    malformed.rays[2].range_m = NAN;
    profile_rejects = !s5_profile_build_config(&malformed, &cfg) && profile_rejects;
    malformed.rays[2].range_m = INFINITY;
    profile_rejects = !s5_profile_build_config(&malformed, &cfg) && profile_rejects;
    malformed.rays[2].range_m = 0.0f;
    profile_rejects = !s5_profile_build_config(&malformed, &cfg) && profile_rejects;
    malformed.rays[2].range_m = 1.0f;
    malformed.rays[2].angle_deg = 60.1f;
    profile_rejects = !s5_profile_build_config(&malformed, &cfg) && profile_rejects;
    malformed.rays[2].angle_deg = 60.0f;
    TEST_CHECK("S5-U07", profile_rejects && s5_profile_build_config(&malformed, &cfg),
               "profile rejects insufficient/oversized scans, invalid Echo range and angles beyond its +/-60 degree bound");
    bool lifecycle = true;
    for (unsigned cancel = 0u; cancel < 2u; ++cancel)
        for (unsigned expired = 0u; expired < 2u; ++expired)
            for (unsigned stationary = 0u; stationary < 2u; ++stationary)
            {
                ScanStatus expected = cancel ? SCAN_CANCELLED :
                    ((expired || !stationary) ? SCAN_FAILED : SCAN_RUNNING);
                lifecycle = lifecycle &&
                    s5_scan_guard(cancel != 0u, expired != 0u, stationary != 0u) == expected;
            }
    TEST_CHECK("S5-U08", lifecycle,
               "scan guard truth table preserves cancel priority across timeout and movement states");
    TEST_SUMMARY("subsystem5");
}
#endif

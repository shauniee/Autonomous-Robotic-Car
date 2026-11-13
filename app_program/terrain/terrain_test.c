/*
 * terrain_test.c - Automated target test runner for Buddy 4.
 */

#include "../app.h"
#include "terrain.h"
#include "../common/robot_data.h"
#include "../test_report.h"

#if APP_TEST
void terrain_test(void)
{
    /* Calibration runs on the I/O task and its own timeout ends it;
       this bound is only a backstop. */
    uint32_t waited = 0u;
    while (!terrain_startup_complete() &&
        waited < TERRAIN_CALIBRATION_TIMEOUT_MS + 500u)
    {
        (void)tk_dly_tsk(10);
        waited += 10u;
    }
    uint32_t word = terrain_read_word();
    uint8_t imu_faults = RD_FAULT_IMU | RD_FAULT_CALIBRATION_FAILED;
    TEST_INIT();
    TEST_CHECK("S4-U01", terrain_calibrated(word) &&
        (rd_fault_get(RD_OWNER_TERRAIN) & imu_faults) == 0u,
        "IMU boot calibration passed, no IMU fault");
    tm_printf((UB*)"[TEST] Terrain State: %d | Hump count: %d\n",
        (int)terrain_state_code(word), (int)terrain_hump_count(word));
    TEST_SUMMARY("terrain");
}
#endif

/*
 * subsystem4_test.c - Automated target test runner for Buddy 4.
 */

#include "../../app_program/subsystem4/subsystem4.h"

#if APP_TEST
void subsystem4_test(void) {
    tm_printf((UB*)"[TEST] === Subsystem 4 (Terrain / GY-511) Bench Check ===\n");
    
    TerrainSnapshot snap;
    ER er = s4_get_terrain(&snap);
    
    if (er == E_OK) {
        tm_printf((UB*)"[TEST] S4 Health: %s | Pitch: %d deg | State: %d\n",
                  snap.terrain_valid ? "VALID" : "SENSOR_ERROR",
                  (int)snap.pitch_deg,
                  (int)snap.state);
        tm_printf((UB*)"[TEST] S4 Max Hump Peak: %d mm (Valid: %d)\n",
                  (int)(snap.max_hump_peak_m * 1000.0f),
                  snap.height_valid);
    } else {
        tm_printf((UB*)"[TEST] S4: Mutex lock failed with code %d\n", er);
    }
    tm_printf((UB*)"[TEST] === S4 Check Complete ===\n");
}
#endif
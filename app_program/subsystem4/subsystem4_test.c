/*
 * subsystem4_test.c - Automated target test runner for Buddy 4.
 */

#include "subsystem4.h"

#if APP_TEST
void subsystem4_test(void) {
    tm_printf((UB*)"[TEST] === Subsystem 4 (Terrain / GY-511) Bench Check ===\n");

    uint32_t word = s4_terrain_word();
    uint32_t max_mm;
    bool max_valid = terrain_code_to_mm(terrain_max_code(word), &max_mm);
    tm_printf((UB*)"[TEST] S4 Calibrated: %d | State: %d | Hump count: %d\n",
              terrain_calibrated(word), (int)terrain_state_code(word), (int)terrain_hump_count(word));
    tm_printf((UB*)"[TEST] S4 Max Hump Peak: %d mm (Valid: %d)\n", max_valid ? (int)max_mm : 0, max_valid);
    tm_printf((UB*)"[TEST] === S4 Check Complete ===\n");
}
#endif

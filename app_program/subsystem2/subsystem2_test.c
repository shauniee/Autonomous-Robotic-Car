#include "subsystem2.h"
#include "../test_report.h"
#if APP_TEST
void subsystem2_test(void) {
    MotionCommand c = {.mode = MOTION_MODE_FOLLOW, .speed_mmps = 100.0f, .action_id = 1u, .sampled_ms = 100u};
    bool pass = motion_command_fresh(&c, 160u) && !motion_command_fresh(&c, 161u) &&
        !motion_command_fresh(&c, 99u + 0x80000000u);
    TEST_INIT();
    TEST_CHECK("S2-U03",pass,"command freshness gate; outputs inhibited");
    TEST_SUMMARY("subsystem2");
}
#endif

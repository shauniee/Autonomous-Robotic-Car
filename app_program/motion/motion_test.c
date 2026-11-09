#include "../app.h"
#include "motion.h"
#include "../test_report.h"
#if APP_TEST
void motion_test(void) {
    MotionCommand c = motion_command_make(MOTION_MODE_FOLLOW, 100.0f, 0.0f, 0, 1u, 100u);
    bool pass = motion_command_fresh(&c, 160u) && !motion_command_fresh(&c, 161u) &&
        !motion_command_fresh(&c, 99u + 0x80000000u);
    TEST_INIT();
    TEST_CHECK("S2-U05", pass, "command freshness gate; outputs inhibited");
    TEST_SUMMARY("motion");
}
#endif

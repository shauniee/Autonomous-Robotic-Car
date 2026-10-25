#include "../../app_program/subsystem2/subsystem2.h"
#include "test_report.h"
#if APP_TEST
void subsystem2_test(void) {
    MotionRequest r={.1f,.1f,100,true};
    bool pass=motion_request_valid(r,false,160) && !motion_request_valid(r,false,161) &&
        !motion_request_valid(r,true,100);
    TEST_INIT();
    TEST_CHECK("S2-U03",pass,"request freshness and stop gate; outputs inhibited");
    TEST_SUMMARY("subsystem2");
}
#endif

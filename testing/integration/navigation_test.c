#include "../../app_program/app.h"
#include "../../app_program/test_report.h"
#include "navigation_fixtures.h"
#if APP_TEST
void robot_navigation_test(void)
{
    TEST_INIT();
    TEST_CHECK("INT-16", robot_navigation_fixture_context(), "event age context generation and config");
    TEST_CHECK("INT-17", robot_navigation_fixture_windows(), "junction distance window and absolute expiry");
    TEST_CHECK("INT-18", robot_navigation_fixture_actions(), "all mapped actions and completion acknowledgement");
    TEST_CHECK("INT-19", robot_navigation_fixture_reacquire(), "two distinct observations bounded route reacquisition");
    TEST_CHECK("INT-20", robot_navigation_fixture_stop(), "local stop dominates navigation and held start");
    TEST_SUMMARY("barcode navigation integration");
}
#endif

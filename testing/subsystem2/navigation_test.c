#include "../../app_program/app.h"
#include "../../app_program/test_report.h"
#include "navigation_fixtures.h"
#if APP_TEST
void subsystem2_navigation_test(void)
{
    TEST_INIT();
    TEST_CHECK("S2-U14", navigation_fixture_quadrature(), "quadrature direction errors and configuration");
    TEST_CHECK("S2-U15", navigation_fixture_actions(), "straight left right U-turn completion and ack");
    TEST_CHECK("S2-U16", navigation_fixture_deadlines(), "renewal preserves deadline count/time wrap");
    TEST_CHECK("S2-U17", navigation_fixture_stall(), "PID bound stationary start stall and invalid encoder");
    TEST_SUMMARY("subsystem2 navigation");
}
#endif

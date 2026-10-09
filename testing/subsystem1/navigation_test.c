#define PROHIBIT_DEF_SIZE_T
#include <stddef.h>
#include "../../app_program/app.h"
#include "../../app_program/test_report.h"
#include "navigation_fixtures.h"
#if APP_TEST
void subsystem1_navigation_test(void)
{
    TEST_INIT();
    TEST_CHECK("S1-U13", s1_navigation_fixture_encode(), "bounded signed speed barcode and null encoding");
    TEST_SUMMARY("subsystem1 navigation observations");
}
#endif

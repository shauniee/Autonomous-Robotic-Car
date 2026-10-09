#include "../../app_program/app.h"
#include "../../app_program/test_report.h"
#include "barcode_fixtures.h"
#if APP_TEST
void subsystem3_barcode_test(void)
{
    TEST_INIT();
    TEST_CHECK("S3-U14", barcode_fixture_symbols(), "A/B/C/D/Z at two scales");
    TEST_CHECK("S3-U15", barcode_fixture_rejection(), "reverse malformed and configuration guards");
    TEST_CHECK("S3-U16", barcode_fixture_events(), "distance widths one-shot rearm and unmapped Z");
    TEST_CHECK("S3-U17", barcode_fixture_overflow(), "event overflow and capture loss latch faults");
    TEST_CHECK("S3-U18", barcode_fixture_wrap(), "count/timer wrap and partial timeout");
    TEST_CHECK("S3-U19", barcode_fixture_capture_ring(), "actual SPSC capacity ordering overflow reset and wrap");
    TEST_SUMMARY("subsystem3 barcode");
}
#endif

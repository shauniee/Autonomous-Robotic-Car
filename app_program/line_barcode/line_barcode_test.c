#include "../app.h"
#include "line_barcode.h"
#include "../common/robot_data.h"
#include "../test_report.h"
#if APP_TEST
void line_barcode_test(void)
{
    /* Line IR is sampled by Mission; wait (bounded) for its first snapshot. */
    LineIrSnapshot s = {0};
    for (int i = 0; i < 50 && !(line_ir_copy(&s) && s.have_sample); i++)
    {
        (void)tk_dly_tsk(10);
    }
    TEST_INIT();
    TEST_CHECK("S3-U01", s.have_sample && ir_raw_word_valid(s.raw) &&
        (rd_fault_get(RD_OWNER_LINE) & RD_FAULT_IR_INVALID) == 0u,
        "line IR sample valid, no IR_INVALID fault");
    TEST_SUMMARY("line_barcode");
}
#endif

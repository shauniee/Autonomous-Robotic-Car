#ifndef IR_SELF_TEST_H
#define IR_SELF_TEST_H

#include <stdbool.h>

/* Pure firmware checks use private fixtures, never the live tracker. */
#if APP_TEST
bool ir_self_test_normalization(void);
bool ir_self_test_guidance(void);
bool ir_self_test_capture(void);
bool ir_self_test_classification(void);
bool ir_self_test_buttons(void);
bool ir_self_test_snapshot(void);
#endif

#endif

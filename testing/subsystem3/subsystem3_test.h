#ifndef SUBSYSTEM3_TEST_H
#define SUBSYSTEM3_TEST_H

#include "../../app_program/subsystem3/subsystem3.h"

#if APP_TEST
ER subsystem3_test_wait_for_start(void);
/* Only the private TEST bridge supplies the mutex; no production ID accessor. */
bool subsystem3_test_snapshot_boundary(ID mutex_id);
#endif

#endif

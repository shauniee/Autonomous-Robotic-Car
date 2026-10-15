#ifndef TEST_REPORT_H
#define TEST_REPORT_H
#include <stdio.h>
extern int host_test_failures;
#define TEST_INIT() int test_pass_ = 0, test_fail_ = 0
#define TEST_CHECK(id, cond, desc) do { if (cond) { test_pass_++; printf("[TEST] %s PASS %s\n", id, desc); } else { test_fail_++; host_test_failures++; printf("[TEST] %s FAIL %s\n", id, desc); } } while (0)
#define TEST_SUMMARY(name) printf("[TEST] %s SUMMARY %d passed, %d failed\n", name, test_pass_, test_fail_)
#endif

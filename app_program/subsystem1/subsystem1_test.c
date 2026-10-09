#define PROHIBIT_DEF_SIZE_T
#include <stddef.h>
#include "subsystem1.h"
#include "../test_report.h"
#include <string.h>
#if APP_TEST
void subsystem1_test(void) {
    S1Telemetry s = {.have_sample=true,.acquired=true,.sampled_ms=100,
        .state="READY",.line="unknown",.capture="waiting"};
    char json[S1_TELEMETRY_PAYLOAD_MAX + 1];
    unsigned length=s1_telemetry_encode(&s,120,json,sizeof(json));
    bool pass=length>0 && strstr(json,"\"sample_age_ms\":20") &&
        strstr(json,"\"fractions\":null") &&
        !s1_telemetry_encode(&s,120,json,16);
    TEST_INIT();
    TEST_CHECK("S1-U04",pass,"bounded telemetry JSON and original age; no radio test");
    TEST_SUMMARY("subsystem1");
}
#endif

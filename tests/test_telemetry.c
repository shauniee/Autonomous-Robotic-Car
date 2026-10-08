#include "../app_program/subsystem1/subsystem1.h"
#include "../app_program/subsystem3/telemetry_s3.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool busy, held;
static unsigned locks, updates;
static ID create_result = 7;
ID tk_cre_mtx(const T_CMTX *m) { assert(m->mtxatr == TA_INHERIT); return create_result; }
ER tk_loc_mtx(ID id,TMO wait) {
    assert(id == 7 && wait == TMO_POL); ++locks;
    if (busy) return E_OBJ;
    assert(!held); held = true; return E_OK;
}
ER tk_unl_mtx(ID id) { assert(id == 7 && held); held = false; return E_OK; }
void tm_printf(const UB *format,...) { (void)format; }

int main(int argc,char **argv) {
    (void)argv;
    char json[S1_TELEMETRY_PAYLOAD_MAX + 1];
    S1Telemetry s = {.sequence=42,.observed_ms=120,.sampled_ms=100,
        .have_sample=true,.acquired=true,.black_set=true,.white_set=true,.reading_valid=true,
        .raw_left=2600,.raw_right=2800,.black_left=3000,.black_right=1000,
        .white_left=1000,.white_right=3000,.left_fraction=800,.right_fraction=100,
        .error=-700,.left_duty=30,.right_duty=170,.motion_enabled=true,.test_inhibited=true,
        .state="FOLLOW_LINE",.line="visible",.reason="ir_proportional_follow",.capture="white_captured"};
    unsigned length = s1_telemetry_encode(&s,120,json,sizeof(json));
    assert(length > 0 && length <= S1_TELEMETRY_PAYLOAD_MAX && strlen(json) == length);
    assert(strstr(json,"\"sample_age_ms\":20") && strstr(json,"\"error\":-0.700"));
    assert(strstr(json,"\"left\":0.030,\"right\":0.170") && strstr(json,"\"test_inhibited\":true"));
    if (argc > 1) { puts(json); return 0; }
    char exact[S1_TELEMETRY_PAYLOAD_MAX + 1];
    assert(s1_telemetry_encode(&s,120,exact,length+1) == length && !strcmp(exact,json));
    assert(!s1_telemetry_encode(&s,120,exact,length) && exact[0] == 0);
    struct { char before, data[16], after; } small = {'A',{0},'Z'};
    assert(!s1_telemetry_encode(&s,120,small.data,sizeof(small.data)));
    assert(small.before == 'A' && small.after == 'Z' && small.data[0] == 0);
    s.sampled_ms=UINT32_MAX-19;
    assert(s1_telemetry_encode(&s,20,json,sizeof(json)) && strstr(json,"\"sample_age_ms\":40"));
    assert(s1_telemetry_encode(&s,41,json,sizeof(json)) && strstr(json,"\"sample_fresh\":false"));
    s=(S1Telemetry){.state="READY",.line="unknown",.reason="capture_black_white",.capture="waiting"};
    assert(s1_telemetry_encode(&s,100,json,sizeof(json)));
    assert(strstr(json,"\"sampled_ms\":null") && strstr(json,"\"raw\":null"));
    assert(strstr(json,"\"fractions\":null,\"error\":null") && strstr(json,"\"black\":null"));
    strcpy(s.reason,"quote\" slash\\ newline\n");
    assert(s1_telemetry_encode(&s,0,json,sizeof(json)) && strstr(json,"quote\\\" slash\\\\ newline\\u000a"));
    memset(s.reason,'x',sizeof(s.reason));
    assert(!s1_telemetry_encode(&s,0,json,sizeof(json)) && !json[0]);
    memset(s.state,1,sizeof(s.state)-1); s.state[sizeof(s.state)-1]=0;
    memset(s.line,1,sizeof(s.line)-1); s.line[sizeof(s.line)-1]=0;
    memset(s.reason,1,sizeof(s.reason)-1); s.reason[sizeof(s.reason)-1]=0;
    memset(s.capture,1,sizeof(s.capture)-1); s.capture[sizeof(s.capture)-1]=0;
    assert(!s1_telemetry_encode(&s,0,json,sizeof(json))); /* Escape expansion exceeds bound. */

    assert(!s1_telemetry_copy(&s) && !s1_telemetry_update(&s) && !locks);
    create_result=E_OBJ; assert(subsystem1_init()==E_OK);
    assert(!s1_telemetry_copy(&s) && !locks); /* Optional object failure does not block startup. */
    create_result=7; assert(subsystem1_init()==E_OK);
    assert(!s1_telemetry_copy(&s));
    s=(S1Telemetry){.sampled_ms=100,.observed_ms=120,.state="READY"};
    assert(s1_telemetry_update(&s)); ++updates;
    S1Telemetry copy;
    assert(s1_telemetry_copy(&copy) && copy.sequence==updates && copy.sampled_ms==100);
    s.sampled_ms=200; busy=true;
    assert(!s1_telemetry_update(&s) && !s1_telemetry_copy(&copy));
    busy=false;
    assert(s1_telemetry_copy(&copy) && copy.sequence==updates && copy.sampled_ms==100);
    assert(s1_telemetry_update(&s)); ++updates;
    s.sampled_ms=999; /* Producer storage is not retained by pointer. */
    assert(s1_telemetry_copy(&copy) && copy.sequence==updates && copy.sampled_ms==200);
    for (unsigned i=0;i<1000;++i) {
        s.raw_left=(uint16_t)i; s.raw_right=(uint16_t)(4095-i);
        assert(s1_telemetry_update(&s) && s1_telemetry_copy(&copy));
        assert(copy.raw_left+copy.raw_right==4095);
    }

    RobotControl car; robot_control_init(&car);
    car.tracking.calibration=(IrCalibration){{3000,1000},{1000,3000},true,true};
    car.state=ROBOT_RUNNING;
    robot_control_sample(&car,(IrRaw){2600,2800},true,100);
    telemetry_s3_update(&car,120,true);
    assert(s1_telemetry_copy(&copy) && copy.sampled_ms==100 && copy.observed_ms==120);
    assert(copy.error==-700 && copy.left_duty==30 && copy.right_duty==170 && copy.test_inhibited);
    RobotControl unchanged=car; busy=true;
    telemetry_s3_update(&car,130,false);
    assert(!memcmp(&car,&unchanged,sizeof(car)));
    busy=false;
    robot_control_sample(&car,(IrRaw){1800,2200},false,140);
    telemetry_s3_update(&car,140,false);
    assert(s1_telemetry_copy(&copy) && !copy.acquired && !copy.reading_valid);
    assert(!strcmp(copy.state,"STOPPED") && !copy.motion_enabled && !copy.test_inhibited);
    puts("[TEST] S1-U04/U05 PASS bounded JSON, original age, nulls, decimal values, coherent snapshots and nonblocking failure");
}

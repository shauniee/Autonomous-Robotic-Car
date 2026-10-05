#include "robot/sensor_bench.h"
#include "robot/comms_protocol.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static bool accept(CommandInbox *i, const char *text) {
    command_inbox_begin(i, strlen(text), true);
    return command_inbox_data(i, text, strlen(text), true, 100);
}
int main(void) {
    BenchVector v = {0};
    const uint8_t accel[] = {0x80,0x3e,0xc0,0xe0,0,0};
    bench_decode_accel(accel,&v);
    assert(v.raw[0]==1000 && v.raw[1]==-500 && v.x==1 && v.y==-.5f);
    const uint8_t magnetic[] = {0x04,0x4c,0x03,0xd4,0xfb,0xb4};
    assert(bench_decode_magnetic(magnetic,&v));
    assert(v.x==1 && v.y==-1 && v.z==1); /* X,Z,Y ordering / axis-specific gain. */
    const uint8_t saturated[] = {0xf0,0,0,0,0,0};
    assert(!bench_decode_magnetic(saturated,&v));
    float p,r,f;
    assert(bench_tilt(0,0,1,&p,&r) && fabsf(p)<.01f && fabsf(r)<.01f);
    assert(!bench_tilt(0,0,0,&p,&r) && !bench_tilt(NAN,0,1,&p,&r));
    assert(bench_ir_fraction(2000,3000,1000,&f) && f==.5f);
    assert(bench_ir_fraction(1000,1000,3000,&f) && f==1);
    assert(!bench_ir_fraction(2000,2000,2100,&f));
    assert(!bench_echo_distance(0,&f) && !bench_echo_distance(30000,&f));
    assert(bench_echo_distance(5800,&f) && f==1);
    assert(bench_servo_pulse(90)==1500 && bench_servo_pulse(180)==2000 && bench_servo_pulse(1000)==2000);
    CommandInbox inbox; command_inbox_reset(&inbox,"0123456789abcdef");
    assert(!accept(&inbox,"stop"));
    assert(!accept(&inbox,"{\"session\":\"0123456789abcdef\",\"id\":1,\"issued_ms\":100,\"command\":\"start\"}"));
    assert(accept(&inbox,"{\"session\":\"0123456789abcdef\",\"id\":1,\"issued_ms\":100,\"command\":\"status\"}"));
    assert(command_inbox_take(&inbox,100)==COMMAND_NONE);
    assert(!accept(&inbox,"{\"session\":\"0123456789abcdef\",\"id\":1,\"issued_ms\":100,\"command\":\"status\"}"));
    assert(accept(&inbox,"{\"session\":\"0123456789abcdef\",\"id\":2,\"issued_ms\":100,\"command\":\"calibrate_black\"}"));
    assert(!accept(&inbox,"{\"session\":\"0123456789abcdef\",\"id\":3,\"issued_ms\":100,\"command\":\"calibrate_white\"}"));
    CalibrationRequest request=command_inbox_take_calibration(&inbox,100);
    assert(request.kind==CALIBRATION_BLACK&&request.id==2);
    assert(command_inbox_take_calibration(&inbox,100).kind==CALIBRATION_NONE);
    assert(accept(&inbox,"{\"session\":\"0123456789abcdef\",\"id\":3,\"issued_ms\":100,\"command\":\"calibrate_white\"}"));
    assert(command_inbox_take_calibration(&inbox,6000).kind==CALIBRATION_NONE);
    SensorBench s = {.ir_status={true,100},.ir_raw=2048,.servo_enabled=true,.servo_command_deg=90,.servo_pulse_us=1500,
        .accel={.status={true,100},.z=1},.forward={.status={true,100},.angle_deg=90,.distance_m=1,.echo_us=5800}};
    char json[COMMS_TELEMETRY_MAX];
    assert(sensor_bench_encode(json,sizeof(json),&s,100));
    assert(strstr(json,"\"raw_adc\":2048") && strstr(json,"\"connected\":false"));
    assert(strstr(json,"\"yaw_rate_dps\":null") && strstr(json,"\"hump_height_m\":null"));
    FILE *out=fopen("build-host/bench-fixture.json","w"); assert(out); fputs(json,out); fclose(out);
    assert(sensor_bench_encode(json,sizeof(json),&s,2000));
    assert(strstr(json,"\"nominal_voltage_v\":null") && strstr(json,"\"distance_m\":null"));
    assert(!sensor_bench_encode(json,16,&s,100) && json[0]==0);
    puts("Sensor conversions, validity, JSON bounds and status-only protocol checks passed.");
}

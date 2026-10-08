#include "lwip/apps/mqtt.h"
#include "lwip/timeouts.h"
#include "../lib/liblwip/sysdepend/pico_rp2040/mqtt_utk.h"
#include "../app_program/subsystem1/subsystem1.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct mqtt_client_s { bool connected; mqtt_connection_cb_t callback; void *arg; };
static struct mqtt_client_s fake;
static uint32_t now;
static unsigned connect_calls, publish_calls, allocations, timer_count, max_timers;
static bool allocation_failure, busy, held;
static err_t connect_result, publish_result;
static char delivered[S1_TELEMETRY_PAYLOAD_MAX+1];
static mqtt_request_cb_t published_callback;
static void *published_arg;
typedef struct { uint32_t due; sys_timeout_handler handler; void *arg; } Timer;
static Timer timers[4];
uint32_t sys_now(void) { return now; }
void sys_timeout(uint32_t delay,sys_timeout_handler handler,void *arg) {
    assert(delay==500 || delay==3000);
    for(unsigned i=0;i<timer_count;++i) assert(timers[i].handler!=handler || timers[i].arg!=arg);
    assert(timer_count<4); timers[timer_count++]=(Timer){now+delay,handler,arg};
    if(timer_count>max_timers) max_timers=timer_count;
}
void sys_untimeout(sys_timeout_handler handler,void *arg) {
    for(unsigned i=0;i<timer_count;) {
        if(timers[i].handler==handler && timers[i].arg==arg) timers[i]=timers[--timer_count];
        else ++i;
    }
}
static void until(uint32_t end) {
    while(now<end) {
        ++now;
        for(unsigned i=0;i<timer_count;) {
            if((int32_t)(now-timers[i].due)>=0) {
                Timer t=timers[i]; timers[i]=timers[--timer_count]; t.handler(t.arg); i=0;
            } else ++i;
        }
        if(now%2==0) mqtt_utk_start(); /* Real radio loop polls every 2 ms. */
    }
}
int ip4addr_aton(const char *text,ip_addr_t *address) {
    assert(!strcmp(text,"127.0.0.1")); address->value=1; return 1;
}
mqtt_client_t *mqtt_client_new(void) { ++allocations; return allocation_failure ? NULL : &fake; }
int mqtt_client_is_connected(mqtt_client_t *c) { assert(c==&fake); return c->connected; }
err_t mqtt_client_connect(mqtt_client_t *c,const ip_addr_t *address,u16_t port,
                         mqtt_connection_cb_t callback,void *arg,const struct mqtt_connect_client_info_t *info) {
    assert(c==&fake && address->value==1 && port==1883);
    assert(!strcmp(info->client_id,"csc01-s3-pico") && info->keep_alive==60);
    ++connect_calls; c->callback=callback; c->arg=arg; return connect_result;
}
err_t mqtt_publish(mqtt_client_t *c,const char *topic,const void *payload,u16_t length,
                   unsigned qos,unsigned retain,mqtt_request_cb_t callback,void *arg) {
    assert(c==&fake && c->connected && !strcmp(topic,"robot/s3/telemetry"));
    assert(qos==0 && retain==0 && length>0 && length<=768);
    ++publish_calls; published_callback=callback; published_arg=arg;
    if(publish_result==ERR_OK) { memcpy(delivered,payload,length); delivered[length]=0; }
    return publish_result;
}
static void connection(mqtt_connection_status_t status) {
    fake.connected=status==MQTT_CONNECT_ACCEPTED;
    fake.callback(&fake,fake.arg,status);
}
ID tk_cre_mtx(const T_CMTX *m) { assert(m->mtxatr==TA_INHERIT); return 7; }
ER tk_loc_mtx(ID id,TMO wait) {
    assert(id==7 && wait==TMO_POL);
    if(busy) return E_OBJ;
    assert(!held); held=true; return E_OK;
}
ER tk_unl_mtx(ID id) { assert(id==7 && held); held=false; return E_OK; }
void tm_printf(const UB *format,...) { (void)format; }

int main(void) {
    allocation_failure=true; mqtt_utk_start(); assert(allocations==1 && timer_count==1);
    until(2999); assert(allocations==1 && connect_calls==0);
    allocation_failure=false; connect_result=ERR_CONN;
    until(3000); assert(connect_calls==1 && timer_count==1);
    until(5999); assert(connect_calls==1);
    connect_result=ERR_OK; until(6000); assert(connect_calls==2 && timer_count==0);
    until(6500); assert(connect_calls==2); /* Connecting guard. */
    connection(MQTT_CONNECT_REFUSED_SERVER); assert(timer_count==1);
    until(9499); assert(connect_calls==2);
    until(9500); assert(connect_calls==3);
    connection(MQTT_CONNECT_ACCEPTED); assert(publish_calls==0 && timer_count==1); /* Before S1 init. */
    assert(subsystem1_init()==E_OK);
    S1Telemetry snapshot={.have_sample=true,.acquired=true,.sampled_ms=9500,
        .raw_left=1234,.raw_right=2345,.state="READY",.line="unknown",.capture="waiting"};
    assert(s1_telemetry_update(&snapshot));
    until(9999); assert(publish_calls==0);
    until(10000); assert(publish_calls==1 && strstr(delivered,"\"sampled_ms\":9500"));
    assert(strstr(delivered,"\"sample_age_ms\":500") && strstr(delivered,"\"sample_fresh\":false"));
    busy=true; until(10500); assert(publish_calls==1); busy=false;
    memset(snapshot.reason,'X',sizeof(snapshot.reason));
    assert(s1_telemetry_update(&snapshot)); until(11000); assert(publish_calls==1);
    strcpy(snapshot.reason,"capture_black_white"); assert(s1_telemetry_update(&snapshot));
    publish_result=ERR_MEM; until(11500); assert(publish_calls==2 && timer_count==1);
    publish_result=ERR_OK; until(12000); assert(publish_calls==3);
    published_callback(published_arg,ERR_CONN); assert(timer_count==1); /* Async failure is diagnostic only. */
    for(unsigned encounter=0;encounter<25;++encounter) {
        connection(MQTT_CONNECT_DISCONNECTED); assert(timer_count==1);
        snapshot.sampled_ms=now+2500; snapshot.raw_left=(uint16_t)(3000+encounter);
        assert(s1_telemetry_update(&snapshot));
        unsigned before=publish_calls, attempts=connect_calls;
        until(now+2999); assert(publish_calls==before && connect_calls==attempts);
        until(now+1); assert(connect_calls==attempts+1);
        connection(MQTT_CONNECT_ACCEPTED);
        assert(publish_calls==before+1 && timer_count==1);
        char expected[40]; snprintf(expected,sizeof(expected),"\"left\":%u",snapshot.raw_left);
        assert(strstr(delivered,expected)); /* Latest offline snapshot, never a backlog. */
        connection(MQTT_CONNECT_ACCEPTED); assert(timer_count==1); /* Repeated callback cannot multiply timers. */
    }
    assert(max_timers==1);
    puts("[TEST] INT-06 PASS real MQTT adapter: bounded retry, refusal, publication failure, latest reconnect and one timer chain");
}

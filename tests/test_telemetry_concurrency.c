#include "../app_program/subsystem1/subsystem1.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static atomic_bool start;
static unsigned copied;
ID tk_cre_mtx(const T_CMTX *m) { assert(m->mtxatr == TA_INHERIT); return 7; }
ER tk_loc_mtx(ID id, TMO wait) {
    assert(id == 7 && wait == TMO_POL);
    int result = pthread_mutex_trylock(&mutex);
    assert(result == 0 || result == EBUSY);
    return result ? E_OBJ : E_OK;
}
ER tk_unl_mtx(ID id) { assert(id == 7 && !pthread_mutex_unlock(&mutex)); return E_OK; }
void tm_printf(const UB *format, ...) { (void)format; }
static void *producer(void *arg) {
    (void)arg;
    while (!atomic_load_explicit(&start, memory_order_acquire)) { }
    for (unsigned i = 1; i <= 50000; ++i) {
        S1Telemetry s = {.have_sample=true, .acquired=true, .sampled_ms=i, .observed_ms=i+3,
            .raw_left=i%4096, .raw_right=4095-i%4096, .black_set=i%2 != 0,
            .left_duty=i%301, .right_duty=350-i%301, .state="READY"};
        strcpy(s.reason, i%2 ? "odd" : "even");
        (void)s1_telemetry_update(&s);
    }
    return NULL;
}
static void *consumer(void *arg) {
    (void)arg;
    uint32_t previous = 0;
    while (!atomic_load_explicit(&start, memory_order_acquire)) { }
    for (unsigned i = 0; i < 50000; ++i) {
        S1Telemetry s;
        if (!s1_telemetry_copy(&s)) continue;
        assert(s.sequence >= previous); previous = s.sequence;
        assert(s.observed_ms == s.sampled_ms + 3 && s.raw_left == s.sampled_ms%4096);
        assert(s.raw_left + s.raw_right == 4095 && s.left_duty + s.right_duty == 350);
        assert(s.black_set == (s.sampled_ms%2 != 0));
        assert(!strcmp(s.reason, s.black_set ? "odd" : "even"));
        assert(!strcmp(s.state, "READY"));
        ++copied;
    }
    return NULL;
}
int main(void) {
    assert(subsystem1_init() == E_OK);
    S1Telemetry initial = {.have_sample=true, .acquired=true, .observed_ms=3,
        .raw_right=4095, .right_duty=350, .state="READY", .reason="even"};
    assert(s1_telemetry_update(&initial));
    pthread_t writer, reader;
    assert(!pthread_create(&writer, NULL, producer, NULL));
    assert(!pthread_create(&reader, NULL, consumer, NULL));
    atomic_store_explicit(&start, true, memory_order_release);
    assert(!pthread_join(writer, NULL) && !pthread_join(reader, NULL) && copied > 0);
    puts("[TEST] S1-U05 PASS concurrent producer/consumer: coherent whole snapshots with nonblocking try-lock");
}

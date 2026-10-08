#include "subsystem1.h"
#include <stdatomic.h>
/* The kernel network task may run before usermain initializes this object. */
static atomic_int telemetry_mutex;
static S1Telemetry latest;
static bool have_snapshot;
static uint32_t sequence;

ER subsystem1_init(void) {
#if TM_MQTT
    T_CMTX mutex = {0};
    mutex.mtxatr = TA_INHERIT;
    ID id = tk_cre_mtx(&mutex);
    atomic_store_explicit(&telemetry_mutex, id, memory_order_release);
    if (id < E_OK)
        tm_printf((UB*)"telemetry: snapshot unavailable; local control continues\n");
#endif
    return E_OK; /* Optional diagnostics must not inhibit the local bench. */
}
ER subsystem1_start(void) { return E_OK; }

bool s1_telemetry_update(const S1Telemetry *snapshot) {
    ID id = atomic_load_explicit(&telemetry_mutex, memory_order_acquire);
    if (!snapshot || id <= 0 || tk_loc_mtx(id, TMO_POL) < E_OK) return false;
    latest = *snapshot;
    latest.sequence = ++sequence;
    have_snapshot = true;
    return tk_unl_mtx(id) >= E_OK;
}
bool s1_telemetry_copy(S1Telemetry *snapshot) {
    ID id = atomic_load_explicit(&telemetry_mutex, memory_order_acquire);
    if (!snapshot || id <= 0 || tk_loc_mtx(id, TMO_POL) < E_OK) return false;
    bool available = have_snapshot;
    if (available) *snapshot = latest;
    if (tk_unl_mtx(id) < E_OK) return false;
    return available;
}

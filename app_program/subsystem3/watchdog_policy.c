#include "watchdog_policy.h"
bool watchdog_progress(WatchdogPolicy *p, bool ok, uint32_t sample, uint32_t motion,
                       uint32_t seq, uint32_t now) {
    if (!ok || !seq || (uint32_t)(now-sample)>60 || (uint32_t)(now-motion)>60 ||
        (p->have_progress && (sample==p->last_sample || seq==p->last_motion))) return false;
    p->last_sample=sample; p->last_motion=seq; p->have_progress=true;
    return true;
}

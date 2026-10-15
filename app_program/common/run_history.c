#include "run_history.h"
#include <string.h>
void run_history_reset(RunHistory *h, uint32_t epoch)
{
    *h = (RunHistory){.run_epoch = epoch};
}
bool run_history_append(RunHistory *h, RunEvent event)
{
    if (h->count == UINT32_MAX)
    {
        h->overflow = true;
        return false;
    }
    event.id = ++h->count;
    if (h->length == RUN_HISTORY_CAPACITY)
    {
        memmove(h->events, h->events + 1,
                (RUN_HISTORY_CAPACITY - 1u) * sizeof(event));
        --h->length;
        if (h->dropped != UINT32_MAX)
        {
            ++h->dropped;
        }
        else
        {
            h->overflow = true;
        }
    }
    h->events[h->length++] = event;
    return true;
}

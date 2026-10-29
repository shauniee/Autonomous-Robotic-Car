#include "range_filter.h"
#include <stddef.h>

bool s5_range_group_begin(s5_range_group_t *group, unsigned requested,
    unsigned minimum)
{
    if (group == NULL || requested == 0u ||
        requested > S5_RANGE_GROUP_CAPACITY || (requested & 1u) == 0u ||
        minimum == 0u || minimum > requested || minimum <= requested / 2u)
    {
        return false;
    }
    *group = (s5_range_group_t){.requested = (uint16_t)requested,
        .minimum = (uint16_t)minimum};
    return true;
}

bool s5_range_group_complete(const s5_range_group_t *group)
{
    return group != NULL && group->requested != 0u &&
        group->attempts == group->requested;
}

bool s5_range_group_push(s5_range_group_t *group, uint32_t range_mm)
{
    if (group == NULL || group->requested == 0u ||
        group->requested > S5_RANGE_GROUP_CAPACITY ||
        group->attempts >= group->requested)
    {
        return false;
    }
    ++group->attempts;
    if (range_mm > 0u && range_mm < ECHO_RANGE_INVALID)
    {
        group->values[group->accepted++] = (uint16_t)range_mm;
    }
    return true;
}

bool s5_range_group_median(const s5_range_group_t *group, uint16_t *range_mm)
{
    if (range_mm == NULL || !s5_range_group_complete(group) ||
        group->minimum == 0u || group->accepted < group->minimum ||
        group->accepted > S5_RANGE_GROUP_CAPACITY)
    {
        return false;
    }
    uint16_t sorted[S5_RANGE_GROUP_CAPACITY];
    for (unsigned i = 0u; i < group->accepted; ++i)
    {
        unsigned at = i;
        while (at > 0u && sorted[at - 1u] > group->values[i])
        {
            sorted[at] = sorted[at - 1u];
            --at;
        }
        sorted[at] = group->values[i];
    }
    unsigned middle = group->accepted / 2u;
    if ((group->accepted & 1u) != 0u)
    {
        *range_mm = sorted[middle];
    }
    else
    {
        *range_mm = (uint16_t)(((uint32_t)sorted[middle - 1u] +
            sorted[middle] + 1u) / 2u);
    }
    return true;
}

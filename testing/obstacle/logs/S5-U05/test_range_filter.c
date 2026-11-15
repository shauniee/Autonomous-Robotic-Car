#include <assert.h>
#include <stdio.h>
#include "../../../../app_program/obstacle/obstacle.h"

int main(void)
{
    obstacle_range_group_t group = {0};
    uint16_t result = 123u;
    assert(!obstacle_range_group_push(&group, 100u));
    assert(!obstacle_range_group_median(&group, &result) && result == 123u);
    assert(!obstacle_range_group_begin(&group, 0u, 1u));
    assert(!obstacle_range_group_begin(&group, 2u, 1u));
    assert(!obstacle_range_group_begin(&group, 7u, 1u));
    assert(!obstacle_range_group_begin(&group, 3u, 4u));
    assert(obstacle_range_group_begin(&group, 3u, 2u));
    assert(obstacle_range_group_push(&group, 100u));
    assert(obstacle_range_group_push(&group, 8000u)); /* isolated outlier */
    assert(!obstacle_range_group_complete(&group));
    assert(!obstacle_range_group_median(&group, &result));
    assert(obstacle_range_group_push(&group, 102u));
    assert(obstacle_range_group_complete(&group));
    assert(obstacle_range_group_median(&group, &result) && result == 102u);
    assert(!obstacle_range_group_push(&group, 1u));
    assert(obstacle_range_group_median(&group, &result) && result == 102u);
    assert(obstacle_range_group_begin(&group, 5u, 3u));
    const uint32_t missing[] = {100u, 0u, UINT32_MAX, 65536u, 102u};
    for (unsigned i = 0u; i < 5u; ++i)
    {
        assert(obstacle_range_group_push(&group, missing[i]));
    }
    assert(obstacle_range_group_complete(&group));
    assert(!obstacle_range_group_median(&group, &result));
    assert(obstacle_range_group_begin(&group, 3u, 2u));
    assert(obstacle_range_group_push(&group, ECHO_RANGE_INVALID - 1u));
    assert(obstacle_range_group_push(&group, ECHO_RANGE_INVALID - 2u));
    assert(obstacle_range_group_push(&group, ECHO_RANGE_INVALID));
    assert(obstacle_range_group_median(&group, &result) && result == ECHO_RANGE_INVALID - 1u);
    assert(obstacle_range_group_begin(&group, 1u, 1u));
    assert(obstacle_range_group_push(&group, 1u));
    assert(obstacle_range_group_median(&group, &result) && result == 1u);
    puts("range_filter: PASS (outliers, unknown groups, boundaries, bounded lifecycle)");
    return 0;
}

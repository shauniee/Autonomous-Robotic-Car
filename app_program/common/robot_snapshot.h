/*
 *	robot_snapshot.h
 *	Mutex copy for snapshots above 32 bits (obstacle profile, event record,
 *	telemetry aggregate). One priority-ceiling or inheritance mutex per
 *	snapshot; the lock covers only the bounded memcpy, never a wait on
 *	another resource. Needs the kernel, so it is not part of robot_data.h.
 */

#ifndef ROBOT_SNAPSHOT_H
#define ROBOT_SNAPSHOT_H

#include <stdint.h>
#include "../app.h"

#define RD_SNAPSHOT_LOCK_MS 2u	/* bounded wait for the copy */

/* Copies src to dst under mtx. Returns E_OK or the kernel error; dst is left
   untouched on failure. Writer and readers use the same call. */
static inline ER rd_snapshot_copy(ID mtx, void *dst, const void *src, uint32_t size)
{
    ER er = tk_loc_mtx(mtx, RD_SNAPSHOT_LOCK_MS);
    if (er < E_OK) {
        return er;
    }
    uint8_t *to = dst;	/* byte loop: <string.h> would clash with the kernel's size_t */
    const uint8_t *from = src;
    for (uint32_t i = 0u; i < size; ++i) {
        to[i] = from[i];
    }
    ER unlock = tk_unl_mtx(mtx);
    return unlock;
}

#endif /* ROBOT_SNAPSHOT_H */

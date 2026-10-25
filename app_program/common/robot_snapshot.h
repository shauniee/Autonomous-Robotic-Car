/*
 *	robot_snapshot.h
 *	Mutex copy for snapshots above 32 bits (obstacle profile, event record,
 *	telemetry aggregate). One priority-ceiling or inheritance mutex per
 *	snapshot; the lock covers only the bounded memcpy, never a wait on
 *	another resource. Needs the kernel, so it is not part of robot_data.h.
 */

#ifndef ROBOT_SNAPSHOT_H
#define ROBOT_SNAPSHOT_H

#include <stddef.h>
#include <string.h>
#include "../app.h"

#define RD_SNAPSHOT_LOCK_MS 2u	/* bounded wait for the copy */

/* Copies src to dst under mtx. Returns E_OK or the kernel error; dst is left
   untouched on failure. Writer and readers use the same call. */
static inline ER rd_snapshot_copy(ID mtx, void *dst, const void *src, size_t size)
{
    ER er = tk_loc_mtx(mtx, RD_SNAPSHOT_LOCK_MS);
    if (er < E_OK) {
        return er;
    }
    memcpy(dst, src, size);
    ER unlock = tk_unl_mtx(mtx);
    return unlock;
}

#endif /* ROBOT_SNAPSHOT_H */

#ifndef SUBSYSTEM3_INTERNAL_H
#define SUBSYSTEM3_INTERNAL_H

#include "subsystem3.h"

/* Private task-to-publication boundary. Failure skips only the cached copy. */
bool subsystem3_publish(const IrTracker *tracker, uint32_t sequence,
    uint32_t now);
void subsystem3_capture_service(IrTracker *tracker, uint32_t now);
#if APP_TEST
bool subsystem3_snapshot_self_test(void);

#endif

ER subsystem3_barcode_init(void);
ER subsystem3_barcode_start(void);

#endif

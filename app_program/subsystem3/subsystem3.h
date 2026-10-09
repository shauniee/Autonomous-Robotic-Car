#ifndef SUBSYSTEM3_H
#define SUBSYSTEM3_H

#include "../app.h"
#include "ir_snapshot.h"
#include "barcode.h"

#define S3_PRI_LINE 12
#define S3_PRI_BARCODE 14

ER subsystem3_init(void);
ER subsystem3_start(void);
/* Configure before start. Copy/take are task-context, nonblocking reads. */
bool s3_barcode_configure(const S3BarcodeConfig *config);
bool s3_barcode_copy(S3BarcodeSnapshot *snapshot);
bool s3_nav_take(S3NavEvent *event);
bool s3_nav_discard(S3BarcodeResult reason);
/* Coordinator is the sole context writer; purge invalidates pending work. */
void s3_barcode_context(uint32_t context, bool purge);
/* Fresh local reset only: purge and permit clearing a latched barcode fault. */
void s3_barcode_reset(void);
/* Nonblocking coherent copy. False means null, uninitialized, or busy/error;
   Ignore the destination on failure; normalized zero is not validity.
   This API is for task context, never an ISR. */
bool s3_ir_copy(S3IrSnapshot *snapshot);
/* Only the app coordinator interprets buttons. S3 owns GPIO setup. */
bool s3_local_inputs(bool *start_down, bool *stop_down);
/* Single pending capture, task context, never blocks. Cancel is independent
   of the snapshot mutex and invalidates earlier queued captures. */
bool s3_capture_request(IrCaptureKind kind);
void s3_capture_cancel(void);
void s3_capture_allow(bool allowed);

#endif

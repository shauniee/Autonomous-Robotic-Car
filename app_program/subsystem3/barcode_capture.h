#ifndef S3_BARCODE_CAPTURE_H
#define S3_BARCODE_CAPTURE_H
#include "barcode.h"
#include <stdatomic.h>

typedef struct
{
    S3BarcodeEdge edges[S3_BARCODE_EDGE_CAPACITY];
    atomic_uint written, consumed;
    atomic_bool overflow;
} S3BarcodeCapture;
/* One same-core ISR producer, one task consumer. Init/reset excludes that
   local IRQ; snapshots and decoder remain separately mutex protected. */
void s3_barcode_capture_init(S3BarcodeCapture *capture);
bool s3_barcode_capture_push_isr(S3BarcodeCapture *capture,
    const S3BarcodeEdge *edge);
bool s3_barcode_capture_pop_task(S3BarcodeCapture *capture,
    S3BarcodeEdge *edge);
bool s3_barcode_capture_empty(const S3BarcodeCapture *capture);
/* Returns prior loss so a context purge cannot silently clear capture fault. */
bool s3_barcode_capture_reset_task(S3BarcodeCapture *capture);
#endif

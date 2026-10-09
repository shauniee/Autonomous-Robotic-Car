#include "barcode_capture.h"
#include <stddef.h>
void s3_barcode_capture_init(S3BarcodeCapture *c)
{
    atomic_init(&c->written, 0u);
    atomic_init(&c->consumed, 0u);
    atomic_init(&c->overflow, false);
}
bool s3_barcode_capture_push_isr(S3BarcodeCapture *c,
    const S3BarcodeEdge *edge)
{
    if ((c == NULL) || (edge == NULL))
    {
        return false;
    }
    unsigned write = atomic_load_explicit(&c->written, memory_order_relaxed);
    unsigned read = atomic_load_explicit(&c->consumed, memory_order_acquire);
    if ((unsigned)(write - read) >= S3_BARCODE_EDGE_CAPACITY)
    {
        atomic_store_explicit(&c->overflow, true, memory_order_release);
        return false;
    }
    c->edges[write % S3_BARCODE_EDGE_CAPACITY] = *edge;
    atomic_store_explicit(&c->written, write + 1u, memory_order_release);
    return true;
}
bool s3_barcode_capture_pop_task(S3BarcodeCapture *c, S3BarcodeEdge *edge)
{
    if ((c == NULL) || (edge == NULL))
    {
        return false;
    }
    unsigned read = atomic_load_explicit(&c->consumed, memory_order_relaxed);
    unsigned write = atomic_load_explicit(&c->written, memory_order_acquire);
    if (read == write)
    {
        return false;
    }
    *edge = c->edges[read % S3_BARCODE_EDGE_CAPACITY];
    atomic_store_explicit(&c->consumed, read + 1u, memory_order_release);
    return true;
}
bool s3_barcode_capture_empty(const S3BarcodeCapture *c)
{
    return atomic_load_explicit(&c->consumed, memory_order_acquire) ==
        atomic_load_explicit(&c->written, memory_order_acquire);
}
void s3_barcode_capture_reset_task(S3BarcodeCapture *c)
{
    atomic_store_explicit(&c->consumed,
        atomic_load_explicit(&c->written, memory_order_acquire),
        memory_order_release);
    atomic_store_explicit(&c->overflow, false, memory_order_release);
}

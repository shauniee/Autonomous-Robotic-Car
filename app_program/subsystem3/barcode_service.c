#include "subsystem3.h"
#include "barcode_capture.h"
#include "../gpio_capture.h"
#include "../subsystem2/encoder.h"
#include <stdatomic.h>

#define BARCODE_PIN 16u
#define BARCODE_PERIOD_MS 5u
#define BARCODE_BATCH 64u
static S3BarcodeCapture capture;
static atomic_uint context_value, epoch;
static atomic_bool started, reset_requested;
static atomic_int barcode_mutex;
static S3BarcodeDecoder decoder;
static S3BarcodeConfig configuration;

static S3BarcodeEdge observation(uint32_t levels, uint32_t timestamp)
{
    S2EncoderSnapshot encoders = {0};
    bool copied = s2_encoder_copy(&encoders);
    return (S3BarcodeEdge){
        .left_count = encoders.left_count, .right_count = encoders.right_count,
        .captured_us = timestamp, .high = ((levels >> BARCODE_PIN) & 1u) != 0u,
        .encoder_valid = copied && encoders.valid,
        .context = atomic_load_explicit(&context_value, memory_order_acquire),
        .generation = atomic_load_explicit(&epoch, memory_order_acquire)
    };
}
static void barcode_isr(uint32_t levels, uint32_t changed,
    uint32_t ambiguous, uint32_t timestamp)
{
    (void)changed;
    S3BarcodeEdge edge = observation(levels, timestamp);
    edge.ambiguous = ambiguous != 0u;
    (void)s3_barcode_capture_push_isr(&capture, &edge);
}

bool s3_barcode_configure(const S3BarcodeConfig *config)
{
    if (!s3_barcode_config_valid(config) ||
        atomic_load_explicit(&started, memory_order_acquire))
    {
        return false;
    }
    configuration = *config;
    return true;
}
void s3_barcode_context(uint32_t context, bool purge)
{
    /* One writer: application coordinator, task context. ISR only reads.
       Changing epoch invalidates already captured edges and pending events. */
    unsigned previous = atomic_load_explicit(&context_value,
        memory_order_acquire);
    if (purge || (previous != context))
    {
        atomic_store_explicit(&context_value, context, memory_order_release);
        unsigned value = atomic_load_explicit(&epoch, memory_order_acquire);
        atomic_store_explicit(&epoch, value + 1u, memory_order_release);
    }
}
void s3_barcode_reset(void)
{
    atomic_store_explicit(&reset_requested, true, memory_order_release);
    s3_barcode_context(0u, true);
}

bool s3_barcode_copy(S3BarcodeSnapshot *snapshot)
{
    ID mutex = atomic_load_explicit(&barcode_mutex, memory_order_acquire);
    if ((snapshot == NULL) || (mutex <= 0) ||
        (tk_loc_mtx(mutex, TMO_POL) < E_OK))
    {
        return false;
    }
    S3BarcodeSnapshot copy = decoder.status;
    bool current = copy.generation ==
        atomic_load_explicit(&epoch, memory_order_acquire);
    ER error = tk_unl_mtx(mutex);
    if ((error < E_OK) || !current)
    {
        return false;
    }
    *snapshot = copy;
    return true;
}
bool s3_nav_take(S3NavEvent *event)
{
    ID mutex = atomic_load_explicit(&barcode_mutex, memory_order_acquire);
    if ((event == NULL) || (mutex <= 0) ||
        (tk_loc_mtx(mutex, TMO_POL) < E_OK))
    {
        return false;
    }
    S3NavEvent copy;
    bool taken = (decoder.status.generation ==
        atomic_load_explicit(&epoch, memory_order_acquire)) &&
        s3_barcode_decoder_take(&decoder, &copy);
    ER error = tk_unl_mtx(mutex);
    if (taken && (error >= E_OK))
    {
        *event = copy;
        return true;
    }
    return false;
}

bool s3_nav_discard(S3BarcodeResult reason)
{
    ID mutex = atomic_load_explicit(&barcode_mutex, memory_order_acquire);
    if (((reason != S3_BC_CONTEXT) && (reason != S3_BC_STALE)) ||
        (mutex <= 0) || (tk_loc_mtx(mutex, TMO_POL) < E_OK))
    {
        return false;
    }
    ++decoder.status.discarded;
    if (decoder.status.state != S3_BC_FAULT)
    {
        decoder.status.result = reason;
    }
    return tk_unl_mtx(mutex) >= E_OK;
}

static void barcode_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    /* This task runs on core 1 (core 0 in UP), after the TEST gate. */
    s2_encoder_enable_context();
    if (gpio_capture_enable() < E_OK)
    {
        s3_barcode_decoder_fault(&decoder, S3_BC_ENCODER);
        return;
    }
    uint32_t serviced_epoch = atomic_load_explicit(&epoch,
        memory_order_acquire);
    for (;;)
    {
        SYSTIM time;
        if (tk_get_otm(&time) < E_OK)
        {
            return;
        }
        ID mutex = atomic_load_explicit(&barcode_mutex, memory_order_acquire);
        if (tk_loc_mtx(mutex, TMO_POL) >= E_OK)
        {
            unsigned current_epoch = atomic_load_explicit(&epoch,
                memory_order_acquire);
            if (current_epoch != serviced_epoch)
            {
                /* Local IRQ exclusion only touches the same-core SPSC ring.
                   It is not a substitute for cross-core snapshot locking. */
                UINT saved;
                DI(saved);
                s3_barcode_capture_reset_task(&capture);
                EI(saved);
                bool clear = atomic_load_explicit(&reset_requested,
                    memory_order_acquire);
                atomic_store_explicit(&reset_requested, false,
                    memory_order_release);
                s3_barcode_decoder_context(&decoder, current_epoch, clear);
                serviced_epoch = current_epoch;
            }
            if (atomic_load_explicit(&capture.overflow, memory_order_acquire) &&
                decoder.status.configured &&
                (decoder.status.state != S3_BC_FAULT))
            {
                s3_barcode_decoder_fault(&decoder, S3_BC_CAPTURE_OVERFLOW);
            }
            for (unsigned i = 0u; i < BARCODE_BATCH; ++i)
            {
                S3BarcodeEdge edge;
                if (!s3_barcode_capture_pop_task(&capture, &edge))
                {
                    break;
                }
                s3_barcode_decoder_edge(&decoder, &edge);
            }
            /* Poll only when the ring has drained: do not confirm a trailing
               quiet zone while older transitions are still pending. */
            if (s3_barcode_capture_empty(&capture))
            {
                UINT saved;
                DI(saved);
                S3BarcodeEdge current = observation(in_w(0xd0000004u),
                    gpio_capture_time_us());
                bool pending = (in_w(0x400140f8u) & 12u) != 0u;
                EI(saved);
                /* A pending GP16 edge must be captured before quiet-zone
                   confirmation; do not overtake a deferred GPIO interrupt. */
                if (!pending)
                {
                    s3_barcode_decoder_poll(&decoder, &current, time.lo);
                }
            }
            (void)tk_unl_mtx(mutex);
        }
        if (tk_dly_tsk(BARCODE_PERIOD_MS) < E_OK)
        {
            return;
        }
    }
}
ER subsystem3_barcode_init(void)
{
    s3_barcode_capture_init(&capture);
    if (gpio_set_pin(BARCODE_PIN, GPIO_MODE_IN) < E_OK)
    {
        return E_SYS;
    }
    T_CMTX mutex = {0};
    mutex.mtxatr = TA_INHERIT;
    ID id = tk_cre_mtx(&mutex);
    if (id <= 0)
    {
        return id < E_OK ? id : E_SYS;
    }
    atomic_store_explicit(&epoch, 1u, memory_order_release);
    s3_barcode_decoder_init(&decoder, &configuration, 1u);
    atomic_store_explicit(&barcode_mutex, id, memory_order_release);
    return gpio_capture_register(1u << BARCODE_PIN, barcode_isr);
}
ER subsystem3_barcode_start(void)
{
    s3_barcode_decoder_init(&decoder, &configuration,
        atomic_load_explicit(&epoch, memory_order_acquire));
    atomic_store_explicit(&started, true, memory_order_release);
    ID task = app_start_task((FP)barcode_task, S3_PRI_BARCODE,
        TP_PRC2, "Barcode");
    return task < E_OK ? task : E_OK;
}

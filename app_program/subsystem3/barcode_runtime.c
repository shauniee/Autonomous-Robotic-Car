#include "barcode_runtime.h"
#include "barcode_service.h"
#include "drv_barcode.h"
#include "../app.h"
#include <stdatomic.h>

static atomic_int barcode_mutex;
static atomic_uint service_faults;
static atomic_bool cancel_pending;
static barcode_service_t service;
static barcode_provider_t provider;
static bool cancel_held;

static uint32_t irq_save(void)
{
    uint32_t state;
    __asm__ volatile ("mrs %0, primask\ncpsid i" : "=r" (state) :: "memory");
    return state;
}

static void irq_restore(uint32_t state)
{
    __asm__ volatile ("msr primask, %0" :: "r" (state) : "memory");
}

static bool owner_core(void)
{
#if TK_SUPPORT_SMP
    return tk_get_prc() == 2;
#else
    return true;
#endif
}

static uint32_t timer_us(void)
{
    return in_w(0x40054028u); /* TIMERAWL: existing shared 1 us timebase. */
}

static barcode_api_result_t lock(ID *id)
{
    *id = atomic_load_explicit(&barcode_mutex, memory_order_acquire);
    if (*id <= 0)
    {
        return BARCODE_API_UNAVAILABLE;
    }
    return tk_loc_mtx(*id, TMO_POL) >= E_OK ? BARCODE_API_OK :
        BARCODE_API_BUSY;
}

static barcode_api_result_t unlock(ID id, barcode_api_result_t result)
{
    return tk_unl_mtx(id) >= E_OK ? result : BARCODE_API_UNAVAILABLE;
}

static bool disable_capture(void)
{
    uint32_t mask = irq_save();
    drv_barcode_enable(false);
    irq_restore(mask);
    return (provider.set_enabled_task == NULL) ||
        provider.set_enabled_task(provider.user, false);
}

int barcode_runtime_init(void)
{
    T_CMTX definition = {0};
    definition.mtxatr = TA_INHERIT;
    ID id = tk_cre_mtx(&definition);
    if (id <= 0)
    {
        return id < E_OK ? id : E_SYS;
    }
    barcode_service_init(&service);
    drv_barcode_reset(); /* Init before any barcode IRQ can be enabled. */
    atomic_store_explicit(&barcode_mutex, id, memory_order_release);
    return E_OK;
}

barcode_api_result_t s3_barcode_configure(const barcode_config_t *config,
    const barcode_provider_t *adapter)
{
    if (!owner_core() || !barcode_config_valid(config) || (adapter == NULL) ||
        (adapter->read_task == NULL) || (adapter->set_enabled_task == NULL))
    {
        return BARCODE_API_INVALID;
    }
    ID id;
    barcode_api_result_t result = lock(&id);
    if (result != BARCODE_API_OK)
    {
        return result;
    }
    if (s3_barcode_faults() != 0u)
    {
        return unlock(id, BARCODE_API_FAULT);
    }
    if (cancel_held || atomic_load_explicit(&cancel_pending,
            memory_order_acquire) ||
        (service.status.state != BARCODE_DISABLED))
    {
        return unlock(id, BARCODE_API_BUSY);
    }
    if (!disable_capture())
    {
        return unlock(id, BARCODE_API_UNAVAILABLE);
    }
    provider = *adapter;
    bool level;
    barcode_encoder_t encoder;
    uint32_t sampled_us;
    if (!provider.set_enabled_task(provider.user, false) ||
        !drv_barcode_gpio_init() ||
        !provider.read_task(provider.user, &level, &encoder, &sampled_us) ||
        !encoder.valid || ((uint32_t)(sampled_us - encoder.sampled_us) >
            config->max_encoder_age_us) ||
        ((uint32_t)(timer_us() - sampled_us) > config->max_encoder_age_us))
    {
        return unlock(id, BARCODE_API_UNAVAILABLE);
    }
    uint32_t mask = irq_save();
    drv_barcode_reset();
    drv_barcode_mapping_revision(service.status.mapping_revision);
    irq_restore(mask);
    (void)barcode_service_configure(&service, config);
    barcode_service_observe(&service, level, &encoder, sampled_us, false);
    mask = irq_save();
    drv_barcode_enable(true);
    irq_restore(mask);
    if (!provider.set_enabled_task(provider.user, true))
    {
        (void)disable_capture();
        barcode_service_reset(&service, BARCODE_BAD_ENCODER);
        return unlock(id, BARCODE_API_UNAVAILABLE);
    }
    return unlock(id, BARCODE_API_OK);
}

barcode_api_result_t s3_barcode_reset(void)
{
    if (!owner_core())
    {
        return BARCODE_API_INVALID;
    }
    ID id;
    barcode_api_result_t result = lock(&id);
    if (result != BARCODE_API_OK)
    {
        return result;
    }
    bool disabled = disable_capture();
    uint32_t mask = irq_save();
    drv_barcode_reset();
    irq_restore(mask);
    barcode_service_reset(&service, BARCODE_CANCELLED);
    atomic_store_explicit(&service_faults, 0u, memory_order_release);
    return unlock(id, disabled ? BARCODE_API_OK : BARCODE_API_UNAVAILABLE);
}

barcode_api_result_t s3_barcode_mapping_replace(
    const barcode_action_t actions[BARCODE_ALPHABET])
{
    if (actions == NULL)
    {
        return BARCODE_API_INVALID;
    }
    for (unsigned i = 0u; i < BARCODE_ALPHABET; ++i)
    {
        if ((unsigned)actions[i] > (unsigned)BARCODE_U_TURN)
        {
            return BARCODE_API_INVALID;
        }
    }
    ID id;
    barcode_api_result_t result = lock(&id);
    if (result != BARCODE_API_OK)
    {
        return result;
    }
    if (!drv_barcode_empty())
    {
        return unlock(id, BARCODE_API_BUSY);
    }
    bool changed = barcode_service_mapping(&service, actions);
    if (changed)
    {
        /* An IRQ racing this replacement retains its old revision. Such
           records are rejected, never interpreted using the new table. */
        drv_barcode_mapping_revision(service.status.mapping_revision);
    }
    return unlock(id, changed ? BARCODE_API_OK : BARCODE_API_BUSY);
}

uint32_t s3_barcode_faults(void)
{
    return drv_barcode_faults() |
        atomic_load_explicit(&service_faults, memory_order_acquire);
}

barcode_api_result_t s3_barcode_copy(barcode_status_t *status)
{
    if (status == NULL)
    {
        return BARCODE_API_INVALID;
    }
    ID id;
    barcode_api_result_t result = lock(&id);
    if (result != BARCODE_API_OK)
    {
        return result;
    }
    *status = service.status;
    status->fault_bits |= s3_barcode_faults();
    if (status->fault_bits != 0u)
    {
        status->state = BARCODE_FAULT;
        status->have_decode = false;
        if (drv_barcode_faults() != 0u)
        {
            status->result = drv_barcode_fault_reason();
        }
    }
    if (atomic_load_explicit(&cancel_pending, memory_order_acquire))
    {
        status->state = BARCODE_DISABLED;
        status->result = BARCODE_CANCELLED;
        status->have_decode = false;
        status->queued_events = 0u;
    }
    return unlock(id, BARCODE_API_OK);
}

barcode_api_result_t s3_barcode_pop(barcode_event_t *event)
{
    if (event == NULL)
    {
        return BARCODE_API_INVALID;
    }
    ID id;
    barcode_api_result_t result = lock(&id);
    if (result != BARCODE_API_OK)
    {
        return result;
    }
    if (s3_barcode_faults() != 0u)
    {
        return unlock(id, BARCODE_API_FAULT);
    }
    if (atomic_load_explicit(&cancel_pending, memory_order_acquire))
    {
        return unlock(id, BARCODE_API_BUSY);
    }
    bool popped = barcode_service_pop(&service, event);
    return unlock(id, popped ? BARCODE_API_OK : BARCODE_API_EMPTY);
}

void barcode_runtime_service(bool cancel_down)
{
    if (!owner_core())
    {
        return;
    }
    if (cancel_down)
    {
        atomic_store_explicit(&cancel_pending, true, memory_order_release);
        /* Gate ISR immediately, even if a task currently holds our mutex. */
        uint32_t mask = irq_save();
        drv_barcode_enable(false);
        irq_restore(mask);
    }
    ID id;
    if (lock(&id) != BARCODE_API_OK)
    {
        return; /* Bounded skip; ring overflow remains independently visible. */
    }
    if (atomic_load_explicit(&cancel_pending, memory_order_acquire))
    {
        if (!cancel_held)
        {
            (void)disable_capture();
            uint32_t mask = irq_save();
            drv_barcode_reset();
            irq_restore(mask);
            barcode_service_reset(&service, BARCODE_CANCELLED);
            atomic_store_explicit(&service_faults, 0u, memory_order_release);
        }
        cancel_held = cancel_down;
        if (!cancel_down)
        {
            atomic_store_explicit(&cancel_pending, false,
                memory_order_release);
        }
    }
    else
    {
        cancel_held = false;
    }
    if (drv_barcode_faults() != 0u)
    {
        barcode_service_fault(&service, drv_barcode_fault_reason());
    }
    if ((service.status.state != BARCODE_DISABLED) &&
        (service.status.state != BARCODE_FAULT))
    {
        barcode_edge_t edge;
        for (unsigned i = 0u; i < BARCODE_SERVICE_BUDGET; ++i)
        {
            if (!drv_barcode_take(&edge))
            {
                break;
            }
            if (edge.mapping_revision != service.status.mapping_revision)
            {
                barcode_service_reject(&service, BARCODE_MAPPING_CHANGED);
                continue;
            }
            barcode_service_observe(&service, edge.level, &edge.encoder,
                edge.sampled_us, true);
            if (service.status.state == BARCODE_FAULT)
            {
                break;
            }
        }
        /* Avoid polling past queued records. The provider's direct coherent
           read is bounded under local IRQ masking: no kernel/locks. A GPIO
           transition occurring during this interval can cause conservative
           rejection; pending flags still belong to the shared dispatcher. */
        uint32_t mask = irq_save();
        bool empty = drv_barcode_empty();
        bool level = false;
        barcode_encoder_t encoder = {0};
        uint32_t sampled_us = 0u;
        bool read = empty && provider.read_task(provider.user, &level,
            &encoder, &sampled_us);
        irq_restore(mask);
        if (empty)
        {
            uint32_t now = timer_us();
            if (!read || ((uint32_t)(now - sampled_us) >
                service.config.max_encoder_age_us))
            {
                encoder.valid = false;
                sampled_us = now;
            }
            barcode_service_observe(&service, level, &encoder,
                sampled_us, false);
        }
        barcode_service_check_time(&service, timer_us());
    }
    atomic_store_explicit(&service_faults, service.status.fault_bits,
        memory_order_release);
    if (service.status.state == BARCODE_FAULT)
    {
        (void)disable_capture();
    }
    (void)unlock(id, BARCODE_API_OK);
}

void barcode_runtime_shutdown(void)
{
    uint32_t mask = irq_save();
    drv_barcode_enable(false);
    irq_restore(mask);
    if (s3_barcode_reset() != BARCODE_API_OK)
    {
        atomic_store_explicit(&service_faults, BARCODE_FAULT_CAPTURE,
            memory_order_release);
    }
}

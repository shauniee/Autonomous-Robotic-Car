#ifndef BARCODE_RUNTIME_H
#define BARCODE_RUNTIME_H

#include "barcode_types.h"

typedef enum
{
    BARCODE_API_OK,
    BARCODE_API_EMPTY,
    BARCODE_API_BUSY,
    BARCODE_API_UNAVAILABLE,
    BARCODE_API_INVALID,
    BARCODE_API_FAULT
} barcode_api_result_t;

/* S2/application adapter contract, not an implemented S2 API.
   read_task returns GP16 level and coherent encoder context acquired now.
   set_enabled_task gates GP16 edge capture synchronously on core 1; false
   leaves GP16 capture disabled. It must not replace USB/encoder/Echo IRQs.
   Callbacks are bounded/nonblocking, never motor or network operations.
   read_task also runs with local IRQs masked: direct acquisition only,
   no kernel calls, locks, allocation or spin/retry loops.
   ISR dispatcher calls drv_barcode_edge_isr from drv_barcode.h separately.
   Adapter and user data must live until reset/replacement completes. */
typedef struct
{
    bool (*read_task)(void *user, bool *level, barcode_encoder_t *encoder,
        uint32_t *sampled_us);
    bool (*set_enabled_task)(void *user, bool enabled);
    void *user;
} barcode_provider_t;

/* Configure/reset: core-1 task only after subsystem3_init. Configure requires
   disabled state, validated mounting/limits, and an agreed GPIO dispatcher.
   Default is disabled. Reset clears all queued/partial events and faults.
   It does NOT authorize motion or automatically re-enable on GP21 release. */
barcode_api_result_t s3_barcode_configure(const barcode_config_t *config,
    const barcode_provider_t *provider);
barcode_api_result_t s3_barcode_reset(void);
/* Any task, nonblocking. Mapping update accepted only while disabled/idle.
   False/busy copies must be ignored. Exactly one consumer removes events. */
barcode_api_result_t s3_barcode_mapping_replace(
    const barcode_action_t actions[BARCODE_ALPHABET]);
barcode_api_result_t s3_barcode_copy(barcode_status_t *status);
barcode_api_result_t s3_barcode_pop(barcode_event_t *event);
/* Independent aligned atomic fault read: does not need the status mutex. */
uint32_t s3_barcode_faults(void);

/* Private lifecycle/task integration, not consumer operations. */
int barcode_runtime_init(void);
void barcode_runtime_service(bool cancel_down);
void barcode_runtime_shutdown(void);

#endif

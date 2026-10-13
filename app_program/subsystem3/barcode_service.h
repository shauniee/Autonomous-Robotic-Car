#ifndef BARCODE_SERVICE_H
#define BARCODE_SERVICE_H

#include "barcode_decode.h"

/* Private engine. One task owner or serialized task callers; never ISR. */
typedef struct
{
    barcode_config_t config;
    barcode_status_t status;
    barcode_action_t mapping[BARCODE_ALPHABET];
    barcode_event_t events[BARCODE_EVENT_CAPACITY];
    barcode_encoder_t previous;
    barcode_encoder_t frame_origin;
    uint32_t widths[BARCODE_ELEMENTS];
    uint32_t run_um;
    uint32_t frame_travel_um;
    uint32_t frame_started_us;
    uint32_t last_edge_us;
    uint32_t last_observed_us;
    uint32_t encounter_id;
    uint8_t count;
    uint8_t event_head;
    int travel_direction;
    bool have_position;
    bool black;
    bool pending_frame;
} barcode_service_t;

void barcode_service_init(barcode_service_t *service);
bool barcode_service_configure(barcode_service_t *service,
    const barcode_config_t *config);
void barcode_service_reset(barcode_service_t *service,
    barcode_result_t reason);
bool barcode_service_mapping(barcode_service_t *service,
    const barcode_action_t actions[BARCODE_ALPHABET]);
void barcode_service_observe(barcode_service_t *service, bool level,
    const barcode_encoder_t *encoder, uint32_t sampled_us, bool edge);
void barcode_service_check_time(barcode_service_t *service, uint32_t now_us);
void barcode_service_fault(barcode_service_t *service,
    barcode_result_t reason);
void barcode_service_reject(barcode_service_t *service,
    barcode_result_t reason);
bool barcode_service_pop(barcode_service_t *service, barcode_event_t *event);

#endif

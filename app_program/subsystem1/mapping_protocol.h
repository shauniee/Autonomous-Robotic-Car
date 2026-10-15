#ifndef MAPPING_PROTOCOL_H
#define MAPPING_PROTOCOL_H
#include "../subsystem3/barcode_types.h"
#define S1_MAPPING_MAX 512u
typedef struct
{
    char payload[S1_MAPPING_MAX + 1u];
    uint32_t request_id;
    unsigned expected, received;
    bool receiving;
    barcode_action_t mapping[BARCODE_ALPHABET];
} S1MappingRequest;
void s1_mapping_reset(S1MappingRequest *request);
bool s1_mapping_begin(S1MappingRequest *request, unsigned length);
bool s1_mapping_data(S1MappingRequest *request, const void *data,
                     unsigned length, bool last);
#endif

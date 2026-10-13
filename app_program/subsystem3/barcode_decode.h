#ifndef BARCODE_DECODE_H
#define BARCODE_DECODE_H

#include "barcode_types.h"

bool barcode_config_valid(const barcode_config_t *config);
/* Pure, bounded decoder. Failure leaves destinations unchanged. */
barcode_result_t barcode_decode(const uint32_t widths[BARCODE_ELEMENTS],
    const barcode_config_t *config, char *symbol,
    barcode_orientation_t *orientation);

#endif

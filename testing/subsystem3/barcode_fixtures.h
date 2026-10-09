#ifndef BARCODE_FIXTURES_H
#define BARCODE_FIXTURES_H
#include "../../app_program/subsystem3/barcode.h"
S3BarcodeConfig barcode_fixture_config(void);
void barcode_fixture_widths(uint16_t middle, unsigned scale,
    uint32_t widths[S3_BARCODE_WIDTHS]);
bool barcode_fixture_symbols(void);
bool barcode_fixture_rejection(void);
bool barcode_fixture_events(void);
bool barcode_fixture_overflow(void);
bool barcode_fixture_wrap(void);
bool barcode_fixture_capture_ring(void);
void subsystem3_barcode_test(void);
#endif

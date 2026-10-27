/* S3 minimum-bit sizeof probe. Compile-only: run sizeof_probe.sh, which
   builds this for Cortex-M0+ with the firmware's target flags and prints
   each probe symbol's size in bytes. Never linked into firmware. */
#include "../../app_program/subsystem3/drv_barcode.h"
#include "../../app_program/subsystem3/barcode_decode.h"
#include "../../app_program/subsystem3/ir_tracking.h"

#define PROBE(name, n) char probe_##name[(n)]
#define SIZE(type) PROBE(sizeof_##type, sizeof(type))
#define FIELD(type, field) PROBE(field_##type##__##field, sizeof(((type *)0)->field))

SIZE(barcode_edge_t);
FIELD(barcode_edge_t, left_count);
FIELD(barcode_edge_t, right_count);
FIELD(barcode_edge_t, high);

SIZE(barcode_event_t);
FIELD(barcode_event_t, left_count);
FIELD(barcode_event_t, right_count);
FIELD(barcode_event_t, symbol);
FIELD(barcode_event_t, action);
FIELD(barcode_event_t, orientation);

SIZE(barcode_frame_t);
FIELD(barcode_frame_t, prev_left);
FIELD(barcode_frame_t, prev_right);

SIZE(IrRaw);
SIZE(IrReading);
FIELD(IrReading, valid);
FIELD(IrReading, left_fraction);
FIELD(IrReading, right_fraction);
FIELD(IrReading, error);

SIZE(IrTrackingOutput);
FIELD(IrTrackingOutput, error);
FIELD(IrTrackingOutput, steering);

SIZE(IrTracker);
FIELD(IrTracker, raw);
FIELD(IrTracker, sample_valid);

SIZE(S3IrSnapshot);
FIELD(S3IrSnapshot, raw);
FIELD(S3IrSnapshot, normalized_left);
FIELD(S3IrSnapshot, normalized_right);
FIELD(S3IrSnapshot, error);
FIELD(S3IrSnapshot, steering);
FIELD(S3IrSnapshot, acquisition_valid);

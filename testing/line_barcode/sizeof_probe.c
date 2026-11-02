/* S3 minimum-bit sizeof probe. Compile-only: run sizeof_probe.sh, which
   builds this for Cortex-M0+ with the firmware's target flags and prints
   each probe symbol's size in bytes. Never linked into firmware. */
#include "../../app_program/line_barcode/barcode.h"
#include "../../app_program/line_barcode/line.h"

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
FIELD(barcode_event_t, packed);

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

SIZE(IrTracker);
FIELD(IrTracker, raw);

SIZE(LineIrSnapshot);
FIELD(LineIrSnapshot, raw);
FIELD(LineIrSnapshot, left_fraction);
FIELD(LineIrSnapshot, right_fraction);
FIELD(LineIrSnapshot, error);

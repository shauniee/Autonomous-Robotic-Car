#include "robot/subsystems.h"

void line_barcode_init(void) {
    /* TODO Buddy 3: initialise three IR inputs; load measured calibration. */
}

LineReading line_barcode_update(uint32_t now_ms) {
    (void)now_ms;
    /* TODO: acquire IR samples, normalise, estimate error, detect junctions.
       Barcode widths should account for measured travel speed/distance.
       Emit navigation once per decoded barcode, not once per loop. */
    return (LineReading){0}; /* Invalid until implemented and calibrated. */
}

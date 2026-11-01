/*
 *	barcode.h
 *	Barcode scanner owned by the S3 task (core 1): edge ring -> frame
 *	assembler -> Code 39 A-Z decoder -> mapped action -> small event queue.
 *	Boots UNMAPPED and disabled; start is refused until a mapping of four
 *	distinct letters (left, right, straight, U-turn) is installed.
 */

#ifndef BARCODE_H
#define BARCODE_H

#include "barcode_decode.h"

/* Core-1 task only: claim GP16 on the dispatcher. False on failure. */
bool barcode_init(void);

/* Install four distinct letters A..Z, indexed by rd_map_action_t. Refused
   while scanning or if the letters are invalid; true on success. */
bool barcode_set_mapping(const uint8_t letters[4]);

/* True once a valid four-letter mapping is installed. */
bool barcode_mapped(void);

/* Mission only. Start is refused (false) until a mapping is installed; it
   drops stale events and asks the S3 task to rearm. The S3 task applies
   start/stop at its next release, resetting the frame and the barcode
   overflow bits. */
bool barcode_start(void);
void barcode_stop(void);

/* S3 task, once per release: apply start/stop, drain the ring with a fixed
   edge budget, assemble and decode frames, queue events, map overflow to the
   Line fault byte. cancel (GP21 held) disables scanning while held. */
void barcode_service(bool cancel);

/* Exactly one consumer removes events. False when empty. */
bool barcode_pop(barcode_event_t *event);

/* S3-task-only optional diagnostic ring. Never called from ISR/Mission. */
typedef struct {
    uint16_t left, right, width;
    uint16_t result : 3;
    uint16_t orientation : 1;
    uint16_t letter : 5; /* 0..25, 31 unavailable */
    uint16_t action : 3;
} barcode_diagnostic_t;
bool barcode_diagnostic_pop(barcode_diagnostic_t *record, unsigned *dropped);
#endif

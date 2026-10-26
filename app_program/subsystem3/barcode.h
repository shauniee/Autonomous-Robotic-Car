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

/* Refused (false) until a mapping is installed. Clears the ring, any partial
   frame, queued events and the barcode fault bits. */
bool barcode_start(void);
void barcode_stop(void);

/* Task, once per release: drain the ring with a fixed edge budget, assemble
   and decode frames, queue events, map capture overflow to the fault byte.
   cancel (GP21) stops scanning. */
void barcode_service(bool cancel);

/* Exactly one consumer removes events. False when empty. */
bool barcode_pop(barcode_event_t *event);

#endif

/*
 *	safety.h
 *	Safety task (core 1, outranks every supervised task). Each 10 ms release
 *	it reads GP21, every owner fault byte and the freshness of the required
 *	producers, and latches stop through s2_stop() on the first cause. It is
 *	the only caller that releases the latch (s2_reset_stop()).
 */

#ifndef SAFETY_H
#define SAFETY_H

#include "../app.h"
#include "safety_logic.h"

ER safety_start(void);

/* Latched causes (SAFETY_CAUSE_*), 0 when not latched. Single load. */
uint32_t safety_latched_causes(void);

/* Ask Safety to release the latch on its next release. Mission calls this on
   a fresh GP20 press. Honoured only when safety_reset_allowed(); poll
   safety_latched_causes() for the result. Never starts motion. */
void safety_request_reset(void);

#endif /* SAFETY_H */

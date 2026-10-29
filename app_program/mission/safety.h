/* Bounded Mission safety policy; independent output guard lives in Motion. */
#ifndef SAFETY_H
#define SAFETY_H

#include "../app.h"
#include "safety_logic.h"

void safety_step(bool line_required);
uint32_t safety_latched_causes(void);

#endif /* SAFETY_H */

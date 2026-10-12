#ifndef BUZZER_H
#define BUZZER_H

#include <stdbool.h>
#include <stdint.h>

/* Initialize once before use.
   All calls must come from the same owning task/startup context. */
bool buzzer_init(void);

/* Request 1–3 chirps. Rejects requests while busy. */
bool buzzer_request(unsigned chirps, uint32_t now);

/* Call periodically to advance the sequence without blocking. */
bool buzzer_service(uint32_t now);

#endif
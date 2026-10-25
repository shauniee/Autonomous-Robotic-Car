/* Jia Jing: obstacle. Front ultrasonic range on the GPIO dispatcher; servo scan follows in #49. */
#ifndef SUBSYSTEM5_H
#define SUBSYSTEM5_H
#include "../app.h"
#include "echo_range.h"

ER subsystem5_init(void);
ER subsystem5_start(void);

/* Front-range word (see echo_range.h): one load, any task. 0 before the first
   reading; a word with valid = 0 means no echo, never clear space. */
uint32_t s5_front_word(void);
#endif

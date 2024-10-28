#ifndef _CONSOLE_ASCII_H
#define _CONSOLE_ASCII_H 1

#include "kbd/kbd.h"

/*
 * Get ascii representation of key event
 * Returns '\0' (ascii NUL) if no representation is present
 */
uint8_t kbd_event_to_ascii(kbd_event_t e);

#endif

#ifndef _DRIVERS_PS2KBD
#define _DRIVERS_PS2KBD 1

#include <stdint.h>
#include <stdbool.h>

#include "drivers/ps2.h"

/*
 * Initialize PS2 keyboard
 * Sets driver passed as pointer
 * Returns false if initialization failed
 */
bool ps2kbd_init(ps2_callbacks *driver, ps2_port port);

#endif
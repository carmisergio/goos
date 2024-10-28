#ifndef _DRIVERS_PS2
#define _DRIVERS_PS2 1

#include "stdint.h"

/*
 * This file defines the standard interface between a PS2 port controller
 * and a PS2 device driver
 */

// Represents the primitives of a PS2 port
typedef struct
{
    // Send byte to PS2 port
    void (*send_data)(uint8_t);

    // Control port state
    void (*enable)();
    void (*disable)();
} ps2_port;

// Represents the primitives of a PS2 driver
typedef struct
{
    // Process byte received from PS2 port
    void (*got_data_callback)(uint8_t);
} ps2_callbacks;

#endif
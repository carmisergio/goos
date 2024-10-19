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
} ps2_port;

// Represents the primitives of a PS2 driver
typedef struct
{
    // Process byte received from PS2 port
    uint8_t (*got_data)();
} ps2_driver;

#endif
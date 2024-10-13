#ifndef _INT_PIT
#define _INT_PIT 1

#include <stdint.h>

#define PIT_FREQ 1193182 // Hz

// PIT Channels
typedef enum
{
    PIT_CHANNEL_0 = 0,
    PIT_CHANNEL_1 = 1,
    PIT_CHANNEL_2 = 2,
} pit_channel_t;

// PIT Modes
typedef enum
{
    // Mode 0: Interrupt on terminal count
    PIT_MODE_0 = 0b000,
    // Mode 1: Hardware re-triggerable one-shot
    PIT_MODE_1 = 0b001,
    // Mode 2: Rate generator
    PIT_MODE_2 = 0b010,
    // Mode 3: Square wave generator
    PIT_MODE_3 = 0b011,
    // Mode 4: Software triggered strobe
    PIT_MODE_4 = 0b100,
    // Mode 5: Hardwae triggered strobe
    PIT_MODE_5 = 0b101,
} pit_mode_t;

/*
 * Set up one channel of the Programmable Interrupt Controller
 * to a specific mode and reset value
 * #### Parameters
 *   - pid_channel_t channel: PIT channel to set up
 *   - pid_mode_t mode: operating mode
 *   - uint16_t reset: reset value
 */
void pit_setup_channel(pit_channel_t channel, pit_mode_t mode, uint16_t reset);

#endif
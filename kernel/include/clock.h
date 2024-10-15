#ifndef _CLOCK
#define _CLOCK 1

#include <stdint.h>

/*
 * Initialise system clock
 */
void clock_init();

/*
 * Get current system clock value in milliseconds
 * #### Returns: uint64_t milliseconds
 */
uint64_t clock_get_system();

/*
 * Get current local time seconds
 * #### Returns: uint32_t seconds
 */
uint32_t clock_get_local();

/*
 * Set offset local time in seconds
 * Internally, this computes an offset between the current time and system
 * time, which then gets applied every time clock_get_local() is called
 * #### Parameters:
 *   - uint32_t time: current local time (s)
 */
void clock_set_local(uint32_t time);

/*
 * Block until specified time has elapsed
 * #### Parameters:
 *   - uint32_t time: time to wait
 */
void clock_delay_ms(uint32_t time);

// Clock IRQ
// DO NOT call this function outside the timer IRQ handler!
void clock_handle_timer_irq();

#endif
#ifndef _CLOCK
#define _CLOCK 1

#include <stdint.h>
#include <stdbool.h>

// Timer handle
typedef int timer_handle_t;

// Timer types
typedef enum
{
    TIMER_ONESHOT,
    TIMER_INTERVAL,
} timer_type_t;

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

/*
 * Set timer
 * #### Parameters:
 *   - type: oneshot or interval
 *   - duration: time in milliseconds
 *   - func:function to call when the timeout expires
 *   - data: poniter to data that gets passed to the function
 *      NOTE:the pointer is never dereferenced. Go nuts!
 * #### Returns: timer handle of the newly set timer, a negative value
 *              if the timeout could not be created (max number of
 *              active timeout exceeded)
 */
timer_handle_t clock_set_timer(uint64_t duration, timer_type_t type,
                               void (*func)(void *), void *data);

/*
 * Clear timer
 * Clears the timer if it was set, does nothing otherwise
 * #### Parameters:
 *   - handle: handle of the timer to reset
 */
void clock_clear_timer(timer_handle_t handle);

/*
 * Reset timer to a new duration
 * Causes the timer to restart from 0
 * #### Parameters:
 *   - handle: timeout handle of the timeout to reset
 *   - duration: new duration
 * #### Returns false if the timeout could not be restarted
 *      (timeout was elapsed before)
 */
bool clock_reset_timer(timer_handle_t handle, uint64_t duration);

/*
 * Check if a given timer is active
 * #### Parameters:
 *   - handle: timeout handle of the timeout to reset
 *   - duration: new duration
 * #### Returns false if the timeout could not be restarted
 *      (timeout was elapsed before)
 */
bool clock_is_timer_active(timer_handle_t handle);

#endif
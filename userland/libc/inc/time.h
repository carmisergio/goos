#pragma once

#include <stdint.h>

// Time type
typedef uint32_t time_t;

/*
 * Get time in seconds
 * #### Parameters:
 *   - tloc: does nothing, should be NULL
 * #### Returns: time in seconds
 */
time_t time(time_t *tloc);

/*
 * Sleep seconds
 */
void sleep(time_t time);

/*
 * Sleep milliseconds
 */
void msleep(uint32_t ms);

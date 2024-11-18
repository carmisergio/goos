#pragma once

#define RAND_MAX 32767

/*
 * Generate pseudo-random integer in the range 0 - RAND_MAX
 * #### Returns pseudo-random number
 */
int rand();

/*
 * Seed pseudo-random number generator
 * #### Parameters:
 *   - seed: new seed value
 */
void srand(unsigned int seed);

/*
 * Cause normal process termination
 */
void exit(int status);
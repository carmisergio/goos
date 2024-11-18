#include "stdlib.h"

/*
 * Rudimentary rand() implementation
 * This implementaion is lifted directly from the POSIX1.2001 standard
 */

// pRNG state
static unsigned long next = 1;

int rand()
{
    next = next * 1103515245 + 12345;
    return ((unsigned)(next / 65536) % RAND_MAX);
}

void srand(unsigned int seed)
{
    next = seed;
}
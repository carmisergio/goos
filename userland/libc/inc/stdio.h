#pragma once

#include <stddef.h>
#include <stdint.h>

/*
 * Write string to system console
'*/
int puts(const char *s);

/*
 * Get string of maximum given size from system console
 */
char *getsn(char *buf, size_t n);

/*
 * Everyone knows what printf does...
 */
void printf(const char *fmt, ...);

/*
 * Convert decimal string to integer
 */
int32_t strtoi(const char *str);
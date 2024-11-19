#pragma once

#include <stddef.h>
#include <stdint.h>

/*
 * Write string to system console
'*/
int puts(const char *s);

/*
 * Write string to system console without newline
'*/
int putss(const char *s);

/*
 * Get string of maximum given size from system console
 * NOTE: buf must be of at least n + 1 bytes
 */
char *getsn(char *buf, size_t n);

/*
 * Get character from system console
 * NOTE: input is not buffered
 * NOTE: doesn't echo characters
 */
int getchar();

/*
 * Everyone knows what printf does...
 */
void printf(const char *fmt, ...);

/*
 * Convert decimal string to integer
 */
int32_t strtoi(const char *str);
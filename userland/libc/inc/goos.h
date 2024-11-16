#pragma once

/*
 * GOOS system call interface functions
 */

#include <stdint.h>

//// Types

//// System calls

/*
 * Write to the system console
 * Supports ANSI control sequences
 * NOTE: ANSI control sequences MUST be sent completely within
 *       a single call to console_write()
 * #### Parameters:
 *   - str: pointer to the string to print
 *   - n: number of characters
 */
void console_write(char *str, uint32_t n);

/*
 * Read a line from the system console
 * #### Parameters:
 *   - buf: pointer to the buffer where to read the string
 *   - n: maximum number of characters
 */
int32_t console_readline(char *str, uint32_t n);

/*
 * Return control to the parent process
 * #### Parameters:
 *   - status: status code that will be passed to the parent
 */
int32_t exit(int32_t status);

/*
 * Execute child process
 * #### Parameters:
 *   - path: null-terminatd path to the executable
 *   - status: pointer to a variable that will hold
 *             the child process return value
 */
int32_t exec(char *path, int32_t *status);

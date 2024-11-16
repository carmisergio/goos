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

////// System errors
#define E_UNKNOWN -1   // Unknown error
#define E_NOIMPL -2    // Not implemented
#define E_NOENT -3     // No such file or directory
#define E_BUSY -4      // Busy
#define E_TOOMANY -5   // Too many
#define E_NOMP -6      // No mountpoint
#define E_NOFS -7      // No filesystem
#define E_WRONGTYPE -8 // Wrong type
#define E_IOERR -9     // I/O error
#define E_NOMEM -10    // Out of memory
#define E_INCON -11    // Filesystem inconsistency
#define E_NOTELF -12   // Not an ELF file
#define E_ELFFMT -13   // Invalid ELF format
#define E_NOTPERM -14  // Not permitted
#define E_INVREQ -15   // Invalid request
#define E_TERM -15     // Terminated by the system

// Get message string for an error
char *error_get_message(int32_t err);
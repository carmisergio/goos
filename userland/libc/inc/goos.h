#pragma once

/*
 * GOOS system call interface functions
 */

#include <stdint.h>

//// Constants

#define FILENAME_MAX 64
#define PATH_MAX 1024

// File open options
#define FOPT_DIR (1 << 0)   // Want directory from open()
#define FOPT_WRITE (1 << 1) // Want to be able to write to file

//// Types

// File descriptor
typedef int32_t fd_t;

// File type
typedef enum
{
    FTYPE_FILE = 0,
    FTYPE_DIR = 1,
} file_type_t;

// Directory entry
typedef struct
{
    char name[FILENAME_MAX + 1]; // NULL terminated file name
    file_type_t type;            // File or directory
    uint32_t size;               // Size of file
} dirent_t;

//// System calls

/*
 * Get local time
 * #### Returns: current local time in seconds since epoch
 */
uint32_t _g_get_local_time();

/*
 * Delay Milliseconds
 * Blocks execution for a specified amount of time
 * #### Parameters;
 *   - time: time to sleep for
 */
void _g_delay_ms(uint32_t time);

/*
 * Write to the system console
 * Supports ANSI control sequences
 * NOTE: ANSI control sequences MUST be sent completely within
 *       a single call to console_write()
 * #### Parameters:
 *   - str: pointer to the string to print
 *   - n: number of characters
 */
void _g_console_write(const char *str, uint32_t n);

/*
 * Read a line from the system console
 * #### Parameters:
 *   - buf: pointer to the buffer where to read the string
 *   - n: maximum number of characters
 */
int32_t _g_console_readline(char *str, uint32_t n);

/*
 * Get a character from system console
 * Doesn't echo characters
 */
char _g_console_getchar();

/*
 * Return control to the parent process
 * #### Parameters:
 *   - status: status code that will be passed to the parent
 */
int32_t _g_exit(int32_t status);

/*
 * Execute child process
 * #### Parameters:
 *   - path: null-terminatd path to the executable
 *   - status: pointer to a variable that will hold
 *             the child process return value
 */
int32_t _g_exec(const char *path, int32_t *status);

/*
 * Change process current working directory
 * #### Parameters:
 *   - path: null-terminated path
 */
int32_t _g_change_cwd(const char *path);

/*
 * Get curernt working directory of process
 * #### Parameters:
 *   - buf: pointer to a buffer of at least MAX_PATH + 1 bytes
 */
int32_t _g_get_cwd(char *buf);

/*
 * Mount filesystem
 * #### Parameters:
 *   - mp: mount point
 *   - dev: block device name
 *   - fs_type: filesystem type
 */
int32_t _g_mount(uint32_t mp, const char *dev, const char *fs_type);

/*
 * Unmount filesystem
 * #### Parameters:
 *   - mp: mount point
 */
int32_t _g_unmount(uint32_t mp);

/*
 * Open file
 * #### Parameters:
 *   - path: path to file
 *   - fopts: open options
 */
int32_t _g_open(const char *path, uint32_t fopts);

/*
 * Close file
 * #### Parameters:
 *   - fd: file descriptor
 */
int32_t _g_close(fd_t fd);

/*
 * Read from file
 * #### Parameters:
 *   - fd: file descriptor
 *   - buf: buffer to place data
 *   - offset: offset from start in bytes
 *   - n: maximum number of bytes to read
 * NOTE: when the result is < n, no more bytes available
 */
int32_t _g_read(fd_t fd, uint8_t *buf, uint32_t offset, uint32_t n);

/*
 * Read directory entries
 * #### Parameters:
 *   - fd: file descriptor
 *   - buf: buffer to place dentries
 *   - offset: offset from start IN NUMBER OF ENTRIES
 *   - n: maximum number of entries to read
 * NOTE: when the result is < n, no more entries available
 */
int32_t _g_readdir(fd_t fd, dirent_t *buf, uint32_t offset, uint32_t n);

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
#define E_MDCHNG -16   // Media changed

// Get message string for an error
char *error_get_message(int32_t err);
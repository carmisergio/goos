#ifndef _LOG_H
#define _LOG_H 1

#include <stdarg.h>

#define kprintf_MAX_LEN 1024

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * Initialize kernel logging subsystem
     */
    void kprintf_init();

    /*
     * Suppress loggging on the system console
     * This is done right before the init process is loaded
     * val: true to suppress output
     */
    void kprintf_suppress_console(bool val);

    /*
     * Log to kernel log
     * Uses printf-style formatting
     * Writes to the debug serial port and the system console
     *
     * Params:
     *    const char* fmt: format string
     *    ...: format values
     * Returns: void
     */
    void kprintf(const char *fmt, ...);

    /*
     * Log to kernel log
     * Uses printf-style formatting
     * Prints only to debug serial output
     * Writes only to the debug serial port
     *
     * Params:
     *    const char* fmt: format string
     *    ...: format values
     * Returns: void
     */
    void kdbg(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
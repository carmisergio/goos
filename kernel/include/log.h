#ifndef _LOG_H
#define _LOG_H 1

#include <stdarg.h>

#define KLOG_MAX_LEN 1024

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * Initialize kernel logging subsystem
     */
    void klog_init();

    /*
     * Initialize kernel logging subsystem features
     * memory management
     */
    void klog_init_aftermem();

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
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
     *
     * Params: void
     * Returns: void
     */
    void klog_init();

    /*
     * Log to kernel log
     * Uses printf-style formatting
     *
     * Params:
     *    const char* fmt: format string
     *    ...: format values
     * Returns: void
     */
    void klog(const char *fmt, ...);

    /*
     * Log to kernel log
     * Uses printf-style formatting
     * Prints only to debug serial output
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
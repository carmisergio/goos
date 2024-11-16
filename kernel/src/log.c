#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "mini-printf.h"

#include "log.h"
#include "console/console.h"
#include "drivers/serial.h"

#define LOG_PORT COM1

// Global objects
bool console_output_enabled;

/* Public functions */

void kprintf_init()
{
    console_output_enabled = true;
    serial_init(LOG_PORT);
}

void kprintf_suppress_console(bool val)
{
    console_output_enabled = !val;
}

void kprintf(const char *fmt, ...)
{
    va_list args;
    char buf[kprintf_MAX_LEN];

    // Initialize valist for n Parameters
    va_start(args, fmt);

    // Format
    mini_vsnprintf(buf, kprintf_MAX_LEN, fmt, args);

    // Print to serial and VGA
    serial_prtstr(LOG_PORT, buf);

    if (console_output_enabled)
        console_write(buf, strlen(buf));

    // Clean up valist
    va_end(args);
}

void kdbg(const char *fmt, ...)
{
    va_list args;
    char buf[kprintf_MAX_LEN];

    // Initialize valist for n Parameters
    va_start(args, fmt);

    // Format
    mini_vsnprintf(buf, kprintf_MAX_LEN, fmt, args);

    // Print to serial
    serial_prtstr(LOG_PORT, buf);

    // Clean up valist
    va_end(args);
}
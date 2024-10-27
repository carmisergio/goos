#include <string.h>
#include <stdint.h>
#include "mini-printf.h"

#include "log.h"
#include "console/console.h"
#include "drivers/serial.h"

#define LOG_PORT COM1

enum arg_len
{
    LEN_DEFAULT,
    LEN_LONG,
    // LEN_LONG_LONG,
    //  LEN_SHORT,
    //  LEN_uint8_t,
};

/* Public functions */

void kprintf_init()
{
    serial_init(LOG_PORT);
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
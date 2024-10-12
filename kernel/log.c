#include <string.h>
#include <stdint.h>
#include "mini-printf.h"

#include "log.h"
#include "drivers/serial.h"
#include "drivers/vga.h"

#define LOG_PORT COM1

static void _klog_format(char *buf, const char *fmt, va_list args);
static void _klog_format_int(char **buf, uint64_t val);
static void _klog_format_hex(char **buf, uint64_t val);
static void _klog_format_str(char **buf, char *str);

enum arg_len
{
    LEN_DEFAULT,
    LEN_LONG,
    // LEN_LONG_LONG,
    //  LEN_SHORT,
    //  LEN_CHAR,
};

/* Public functions */

void klog_init()
{
    // TODO:Check if initialization was succesful
    serial_init(COM1);
    vga_init();
}

void klog(const char *fmt, ...)
{
    va_list args;
    char buf[KLOG_MAX_LEN];

    // Initialize valist for n Parameters
    va_start(args, fmt);

    // // Format
    // _klog_format(buf, fmt, args);
    mini_vsnprintf(buf, KLOG_MAX_LEN, fmt, args);

    // Print to serial and VGA
    serial_prtstr(LOG_PORT, buf);
    vga_prtstr(buf);

    // Clean up valist
    va_end(args);
}

void kdbg(const char *fmt, ...)
{
    va_list args;
    char buf[KLOG_MAX_LEN];

    // Initialize valist for n Parameters
    va_start(args, fmt);

    // Format
    _klog_format(buf, fmt, args);

    // Print to serial
    serial_prtstr(LOG_PORT, buf);

    // Clean up valist
    va_end(args);
}

/* Internal functions */

/*
 * Render format string into buffer
 * Follows printf-style formatting
 *
 * Params:
 *     char *buf: buffer into which to output formatted string
 *     const char *fmt: format string
 *     va_list args: list of which to format the values
 * Returns: void
 */
static void _klog_format(char *buf, const char *fmt, va_list args)
{
    // Process fmt
    while (*fmt)
    {

        // Character is not format specifier
        if (*fmt != '%')
        {
            *buf = *fmt;
            buf++;
            fmt++;
            continue;
        }
        else
        {
            fmt++;
        }

        // Evaluate length field
        enum arg_len len = LEN_DEFAULT;
        switch (*fmt)
        {
        case 'l':
            len = LEN_LONG;
            fmt++;
            // if (*fmt == 'l')
            // {
            //     len = LEN_LONG_LONG;
            //     fmt++;
            // }
            break;
            // case 'h':
            //     len = LEN_SHORT;
            //     fmt++;
            //     if (*fmt == 'h')
            //     {
            //         len = LEN_CHAR;
            //         fmt++;
            //     }
            //     break;
        }

        long long val;
        char *str;

        // Evaluate specifier
        switch (*fmt)
        {
        case '%':
            // Percent sign literal
            *buf = '%';
            fmt++;
            buf++;
            break;
        case 'd':
            // Decimal integer
            switch (len)
            {
            case LEN_DEFAULT:
                val = va_arg(args, uint32_t);
                break;
            case LEN_LONG:
                val = va_arg(args, uint64_t);
                break;
            }

            // Format value
            _klog_format_int(&buf, val);
            fmt++;
            break;

        case 'x':
            // Hexadecimal integer
            long long val;
            switch (len)
            {
            case LEN_DEFAULT:
                val = va_arg(args, uint32_t);
                break;
            case LEN_LONG:
                val = va_arg(args, uint64_t);
                break;
            }

            // Format value
            _klog_format_hex(&buf, val);
            fmt++;
            break;

        case 's':
            // String
            str = (char *)va_arg(args, void *);
            _klog_format_str(&buf, str);
            fmt++;
            break;
        }
    }

    // Null-terminate string
    *buf = '\0';
}

/*
 * Render integer to the buffer
 *
 * Params:
 *     char *buf: pointer to pointer to buffer into which to output formatted string
 *     int val: value to format
 * Returns: void
 */
static void _klog_format_int(char **buf, uint64_t val)
{
    int ndigits = 0;

    // Handle 0 case
    if (val == 0)
    {
        **buf = '0';
        (*buf)++;
    }

    // Put digits in buffer in inverted order
    while (val != 0)
    {
        // Get rightmost digit as ASCII character
        char digit = (val % 10) + '0';

        // Remove rightmost for next iteration
        val /= 10;

        // Add digit to buffer
        **buf = digit;
        (*buf)++;

        // Count digit
        ndigits++;
    }

    // Swap order of characters in buffer
    for (size_t i = 0; i < (unsigned int)ndigits / 2; i++)
    {
        // Swap characters
        char tmp = *(*buf - i - 1);
        *(*buf - i - 1) = *(*buf - ndigits + i);
        *(*buf - ndigits + i) = tmp;
    }
}

/*
 * Render hexadecimal integer to the buffer
 *
 * Params:
 *     char *buf: pointer to pointer to buffer into which to output formatted string
 *     int val: value to format
 * Returns: void
 */
static void _klog_format_hex(char **buf, uint64_t val)
{
    int ndigits = 0;

    // Handle 0 case
    if (val == 0)
    {
        **buf = '0';
        (*buf)++;
    }

    // Put digits in buffer in inverted order
    while (val != 0)
    {
        // Get rightmost digit as ASCII character
        uint8_t digit_val = val % 16;
        char digit;
        if (digit_val < 10)
            digit = digit_val + '0';
        else
            digit = digit_val - 10 + 'a';

        // Remove rightmost for next iteration
        val /= 16;

        // Add digit to buffer
        **buf = digit;
        (*buf)++;

        // Count digit
        ndigits++;
    }

    // Swap order of characters in buffer
    for (size_t i = 0; i < (unsigned int)ndigits / 2; i++)
    {
        // Swap characters
        char tmp = *(*buf - i - 1);
        *(*buf - i - 1) = *(*buf - ndigits + i);
        *(*buf - ndigits + i) = tmp;
    }
}

/*
 * Render null-terminated string
 *
 * Params:
 *     char *buf: pointer to pointer to buffer into which to output formatted string
 *     char *str: pointer to the string
 * Returns: void
 */
static void _klog_format_str(char **buf, char *str)
{
    while (*str)
    {
        **buf = *str;
        (*buf)++;
        str++;
    }
}

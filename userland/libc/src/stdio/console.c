#include "stdio.h"

#include "string.h"
#include "goos.h"
#include "mini-printf.h"

#define PRINTF_MAX 1024 // Maximum number of characters from a single printf() call

int puts(const char *s)
{
    _g_console_write(s, strlen(s));
    _g_console_write("\n", 1);
    return 0;
}

int putss(const char *s)
{
    _g_console_write(s, strlen(s));
    return 0;
}

char *getsn(char *buf, size_t n)
{
    int32_t read = _g_console_readline(buf, n);
    buf[read] = 0;
    return buf;
}

int getchar()
{
    return _g_console_getchar();
}

void printf(const char *fmt, ...)
{
    va_list args;
    char buf[PRINTF_MAX];

    // Initialize valist for n Parameters
    va_start(args, fmt);

    // Format
    mini_vsnprintf(buf, PRINTF_MAX, fmt, args);

    // Write string to console
    putss(buf);

    // Clean up valist
    va_end(args);
}
#include <stddef.h>

#include "string.h"

void *memcpy(void *__restrict dst, const void *__restrict src, size_t n)
{
    unsigned char *dstc = (unsigned char *)dst;
    unsigned char *srcc = (unsigned char *)src;

    for (size_t i = 0; i < n; i++)
        dstc[i] = srcc[i];

    return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
    unsigned char *dstc = (unsigned char *)dst;
    unsigned char *srcc = (unsigned char *)src;

    if (dst < src)
    {
        // Copy forwards
        for (size_t i = 0; i < n; i++)
            dstc[i] = srcc[i];
    }
    else
    {
        // Copy backwards
        for (size_t i = 1; i <= n; i++)
            dstc[n - i] = srcc[n - i];
    }

    return dst;
}

void *memset(void *s, int c, size_t n)
{
    unsigned char *sc = (unsigned char *)s;

    for (size_t i = 0; i < n; i++)
        sc[i] = (unsigned char)c;

    return s;
}

int memcmp(const void *aptr, const void *bptr, size_t n)
{
    const unsigned char *a = (const unsigned char *)aptr;
    const unsigned char *b = (const unsigned char *)bptr;
    for (size_t i = 0; i < n; i++)
    {
        if (a[i] < b[i])
            return -1;
        else if (b[i] < a[i])
            return 1;
    }
    return 0;
}

size_t strlen(const char *str)
{
    size_t len = 0;
    while (str[len])
        len++;

    return len;
}
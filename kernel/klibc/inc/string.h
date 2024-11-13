#ifndef _STRING_H
#define _STRING_H 1

#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif

    int memcmp(const void *s1, const void *s2, size_t n);
    void *memcpy(void *__restrict dst, const void *__restrict src, size_t n);
    void *memmove(void *dst, const void *src, size_t n);
    void *memset(void *s, int c, size_t n);
    size_t strlen(const char *str);
    int strcmp(const char *p1, const char *p2);
    char *strcpy(char *dst, const char *src);

#ifdef __cplusplus
}
#endif

#endif

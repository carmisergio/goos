#ifndef _SYS_IO_H
#define _SYS_IO_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %w1, %b0"
                 : "=a"(ret)
                 : "Nd"(port)
                 : "memory");
    return ret;
}

#endif

#ifndef _SYS_IO_H
#define _SYS_IO_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %w1, %b0"
                     : "=a"(ret)
                     : "Nd"(port)
                     : "memory");
    return ret;
}

// Send two bytes to 8 bit IO port
// (low byte first, then high byte)
static inline void outb16_lh(uint16_t port, uint16_t val)
{
    outb(port, val & 0xFF);
    outb(port, (val & 0xFF00) >> 8);
}

// Small delay
static inline void io_delay()
{
    outb(0x80, 0);
}

#endif

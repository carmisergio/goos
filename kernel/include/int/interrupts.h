#ifndef _INT_INTERRUPTS_H
#define _INT_INTERRUPTS_H

#include <stdint.h>

// Hardwaree interrupt vector offset
#define IRQ_VEC_OFFSET 0x20

typedef struct
{
    // Vector saved
    uint32_t edi;
    uint32_t esi;
    uint32_t edx;
    uint32_t ecx;
    uint32_t ebx;
    uint32_t eax;
    uint32_t vec;

    // CPU saved
    uint32_t errco;
    uint32_t eip;
    uint16_t cs;
    uint16_t _res0;
    uint32_t eflags;
} interrupt_context_t;

/*
 * Initialize interrupt handling
 */
void interrupts_init();

#endif
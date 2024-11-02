#ifndef _INT_INTERRUPTS_H
#define _INT_INTERRUPTS_H

#include <stdint.h>

// Hardwaree interrupt vector offset
#define IRQ_VEC_OFFSET 0x20

typedef struct
{
    // Vector saved
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t ebp;
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
    uint32_t esp;
    uint32_t ss;
} interrupt_context_t;

/*
 * Initialize interrupt handling
 */
void interrupts_init();

/*
 * Register handler for an IRQ
 * #### Parameters:
 *   - uint8_t irq: IRQ number
 *   - void (*handler)(): pointer to handler function
 */
void interrupts_register_irq(uint8_t irq, void (*handler)());

/*
 * Unregister handler for an IRQ
 * #### Parameters:
 *   - uint8_t irq: IRQ number
 *   - void (*handler)(): pointer to handler function
 */
void interrupts_unregister_irq(uint8_t irq, void (*handler)());

// Enable hardware interrupts
static inline void sti()
{
    __asm__("sti");
}

// Disable hardware interrupts
static inline void cli()
{
    __asm__("cli");
}

#endif
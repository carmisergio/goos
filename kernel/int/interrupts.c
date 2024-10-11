#include "int/interrupts.h"

#include <stdint.h>
#include "sys/io.h"

#include "int/idt.h"
#include "int/pic.h"
#include "log.h"
#include "panic.h"

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

void interrupts_init()
{
    klog("Initializing interrupts...\n");

    // Load IDT
    set_up_idt();

    // Remap PIC
    pic_init(IRQ_VEC_OFFSET);

    // Enable hardware interrupts
    asm("sti");
}

void interrupt_handler(interrupt_context_t *ctx)
{
    // klog("Interrupt! vector = %d\n", ctx->vec);
    // klog("  Previous EIP: %x\n", ctx->eip);

    if (ctx->vec < 32)
    {
        // Exception
        panic("EXCEPTION");
    }
    else
    {
        // Hardare interrupt

        // Figure out which IRQ was raised
        uint32_t irq = ctx->vec - IRQ_VEC_OFFSET;

        kdbg("[INT] IRQ %d\n", irq);

        // Send End of Interrupt sequence
        pic_int_send_eoi(irq);
    }
}
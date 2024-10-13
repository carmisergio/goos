#include "int/interrupts.h"

#include "sys/io.h"
#include "mini-printf.h"

#include "int/idt.h"
#include "int/exceptions.h"
#include "log.h"
#include "panic.h"
#include "drivers/pic.h"

void interrupts_init()
{
    klog("Initializing interrupts...\n");

    // Load IDT
    set_up_idt();

    // Remap PIC
    pic_init(IRQ_VEC_OFFSET);

    // Enable hardware interrupts
    sti();
}

void interrupt_handler(interrupt_context_t *ctx)
{
    // klog("Interrupt! vector = %d\n", ctx->vec);
    // klog("  Previous EIP: %x\n", ctx->eip);

    if (ctx->vec < 32)
    {
        handle_exception(ctx);
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
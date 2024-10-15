#include "int/interrupts.h"

#include "sys/io.h"
#include "mini-printf.h"

#include "int/idt.h"
#include "int/exceptions.h"
#include "log.h"
#include "panic.h"
#include "drivers/pic.h"
#include "clock.h"

// Internal functions
void handle_irq(uint16_t irq);

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
    if (ctx->vec < 32)
    {
        // CPU exception
        handle_exception(ctx);
    }
    else
    {
        // Hardare interrupt

        // Figure out which IRQ was raised
        uint16_t irq = ctx->vec - IRQ_VEC_OFFSET;

        handle_irq(irq);
    }
}

// Handle hardware interrupt
void handle_irq(uint16_t irq)
{
    switch (irq)
    {
    case 0:
        // Timer interrupt
        clock_handle_timer_irq();
        break;
    }

    // Send End of Interrupt sequence
    pic_int_send_eoi(irq);
}
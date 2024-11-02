#include "int/interrupts.h"

#include "sys/io.h"
#include "mini-printf.h"

#include "int/idt.h"
#include "int/exceptions.h"
#include "log.h"
#include "panic.h"
#include "drivers/pic.h"
#include "clock.h"
#include "syscall/syscall.h"

// #define DEBUG 1

// Maximum IRQ handlers for each IRQ
#define MAX_IRQ_HANDLERS 2

#define NUM_IRQ 16
#define IRQ_UNUSED 0

// Internal functions
void handle_irq(uint16_t irq);

// Global objects
void (*irq_handlers[NUM_IRQ][MAX_IRQ_HANDLERS])(); // Pointers to IRQ handlers

/* Public functions */

void interrupts_init()
{
    kprintf("[INT] Initializing...\n");

    // Clear irq_handlers array
    for (size_t i = 0; i < NUM_IRQ; i++)
    {
        for (size_t j = 0; j < MAX_IRQ_HANDLERS; j++)
            irq_handlers[i][j] = IRQ_UNUSED;
    }

    // Load IDT
    set_up_idt();

    // Remap PIC
    pic_init(IRQ_VEC_OFFSET);

    // Enable hardware interrupts
    sti();
}

void interrupts_register_irq(uint8_t irq, void (*handler)())
{
    // Panic message
    const size_t msg_n = 64;
    char msg[msg_n];

    // Check if irq is valid
    if (irq >= NUM_IRQ)
    {
        mini_snprintf(msg, msg_n,
                      "Tried to register invalid IRQ\n\nIRQ: %d\nHandler: 0x%x",
                      irq, handler);
        panic("INT_REGISTER_IRQ_INVALID_IRQ", msg);
    }

    cli();

    // Find empty slot
    // Always iterate over all slots to check if the handler is already defined
    int32_t slot = -1;
    for (size_t i = 0; i < MAX_IRQ_HANDLERS; i++)
    {
        if (irq_handlers[irq][i] == handler)
        {
            // Handler is already registered
            mini_snprintf(msg, msg_n,
                          "Handler already registered\n\nIRQ: %d\nHandler: 0x%x",
                          irq, handler);
            panic("INT_REGISTER_IRQ_ALREADY_REGISTERED", msg);
        }
        else if (irq_handlers[irq][i] == IRQ_UNUSED)
        {
            // Found empty slot
            slot = i;
        }
    }

    if (slot >= 0)
    {
        // Set handler
        irq_handlers[irq][slot] = handler;
        sti();
    }
    else
    {
        // We couldn't find an empty slot for the IRQ handler
        sti();
        mini_snprintf(msg, msg_n,
                      "No empty slot for IRQ handler\n\nIRQ: %d\nHandler: 0x%x",
                      irq, handler);
        panic("INT_REGISTER_IRQ_NO_EMPTY_SLOT", msg);
    }
}

void interrupts_unregister_irq(uint8_t irq, void (*handler)())
{
    // Panic message
    const size_t msg_n = 64;
    char msg[msg_n];

    // Check if irq is valid
    if (irq >= NUM_IRQ)
    {
        mini_snprintf(msg, msg_n,
                      "Tried to unregister invalid IRQ\n\nIRQ: %d\nHandler: 0x%x",
                      irq, handler);
        panic("INT_UNREGISTER_IRQ_INVALID_IRQ", msg);
    }

    cli();

    for (size_t i = 0; i < MAX_IRQ_HANDLERS; i++)
    {
        if (irq_handlers[irq][i] == handler)
        {
            // Unregister handler
            irq_handlers[irq][i] = IRQ_UNUSED;

            sti();
            return;
        }
    }

    // The handler wasn't registered previously
    sti();
    mini_snprintf(msg, msg_n,
                  "Handler not registered\n\nIRQ: %d\nHandler: 0x%x",
                  irq, handler);
    panic("INT_UNREGISTER_IRQ_HANDLER_NOT_REGISTERED", msg);
}

/* Internal functions */

// Main ISR, called from vectors in vectors.S
void interrupt_handler(interrupt_context_t *ctx)
{
#ifdef DEBUG
    kdbg("[INT] %d\n", ctx->vec);
#endif

    if (ctx->vec < 32)
    {
        // CPU exception
        handle_exception(ctx);
    }
    else if (ctx->vec < 48)
    {
        // Hardware interrupt
        handle_irq(ctx->vec - IRQ_VEC_OFFSET);
    }
    else if (ctx->vec == 48)
    {
        // System call
        handle_syscall(ctx);
    }
}

// Handle hardware interrupt
void handle_irq(uint16_t irq)
{
    // Handle spurious interrupts
    if ((irq == 7 || irq == 15) && pic_check_spurious(irq))
        return;

    // Dispatch IRQs
    for (size_t i = 0; i < MAX_IRQ_HANDLERS; i++)
    {
        if (irq_handlers[irq][i] != IRQ_UNUSED)
            irq_handlers[irq][i]();
    }

    // Send End of Interrupt sequence
    pic_send_eoi(irq);
}
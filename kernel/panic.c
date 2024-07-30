#include <stdbool.h>

#include "panic.h"
#include "drivers/vga.h"
#include "log.h"

// Internal function prototypes
void _panic_halt();
static inline void _cli();
static inline void _hlt();

/* Public functions */

void panic(char *code)
{
    // Clear screen
    vga_setcol(VGA_COLOR_WHITE, VGA_COLOR_RED);
    vga_clearscreen();

    // Print message
    klog("KERNEL PANIC!\n");
    klog("Code: %s\n", code);

    // Halt processor
    _panic_halt();
}

/* Internal functions */

/*
 * Puts CPU in halted state and prevents any further
 * code execution
 */
void _panic_halt()
{
    _cli();
    while (true)
    {
        _hlt();
    }
}

/*
 * Clear interrupts
 */
static inline void _cli()
{
    asm volatile("cli");
}

/*
 * Halt processor
 */
static inline void _hlt()
{
    asm volatile("hlt");
}
#include <stdbool.h>

#include "panic.h"
#include "drivers/vga.h"
#include "log.h"
#include "int/interrupts.h"

// Internal function prototypes
void _panic_halt();
static inline void _hlt();

/* Public functions */

void panic(char *code, char *message)
{
    // Clear screen
    vga_setcol(VGA_COLOR_WHITE, VGA_COLOR_RED);
    vga_clearscreen();

    // Print message
    klog("********************************************************************************\n");
    klog("*                        QUACK! This is a KERNEL PANIC!                        *\n");
    klog("********************************************************************************\n");
    klog("Code: %s\n", code);
    klog("%s\n", message);

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
    cli();
    while (true)
    {
        _hlt();
    }
}

/*
 * Halt processor
 */
static inline void _hlt()
{
    __asm__("hlt");
}
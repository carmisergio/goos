#include <stdbool.h>

#include "panic.h"
#include "drivers/vga.h"
#include "log.h"
#include "int/interrupts.h"
#include "console/console.h"

// Internal function prototypes
void _panic_halt();
static inline void _hlt();

/* Public functions */

void panic(char *code, char *message)
{
    kprintf_suppress_console(false);

    // Clear screen
    console_reset();
    console_set_bgcol(CONS_COL_RED);
    console_set_fgcol(CONS_COL_HI_WHITE);
    console_clear();
    console_set_curspos(0, 0);

    // Print message
    kprintf("********************************************************************************\n");
    kprintf("*                        QUACK! This is a KERNEL PANIC!                        *\n");
    kprintf("********************************************************************************\n");
    kprintf("Code: %s\n", code);
    kprintf("%s\n", message);

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
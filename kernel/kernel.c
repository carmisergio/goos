#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "string.h"

#include "log.h"
#include "console.h"
#include "mem/mem.h"
#include "mem/physmem.h"
#include "mem/vmem.h"
#include "boot/boot_info.h"
#include "panic.h"
#include "boot/multiboot_structs.h"
#include "drivers/vga.h"
#include "drivers/serial.h"
#include "int/interrupts.h"
#include "drivers/pit.h"
#include "drivers/kbdctl.h"
#include "clock.h"
#include "kbd/kbd.h"
#include "sysreq.h"

// Boot information structure
boot_info_t boot_info;

void kbd_handler(kbd_event_t e)
{
    kprintf("Keyboard event: 0x%02x\n", e.keysym);
}

/*
 * Main kernel entry point
 * Remember to initialize boot_info before calling this!
 */
void kmain(multiboot_info_t *mbd)
{
    // Initialize kernel logging
    console_init();
    klog_init();
    kprintf("\e[94mGOOS\e[0m starting...\n");

    // Initialize memory management
    mem_init(mbd);

    // Initialize interrupts
    interrupts_init();

    // Initialize subsystems
    kbd_init();

    // Initialize drivers
    clock_init();
    kbdctl_init();
    sysreq_init();

    kprintf("BOOTED!\n");

    kbd_register_kbd_event_recv(kbd_handler);

    char *str = "\e[31mRED \e[32mGREEN \e[34mBLUE \e[0m\n";
    console_write(str, strlen(str));

    str = "\e[41mRED \e[42mGREEN \e[44mBLUE \e[0m\n";
    console_write(str, strlen(str));

    str = "\e[91;102mTEST\n";
    console_write(str, strlen(str));

    for (;;)
    {
        // uint32_t time = clock_get_local();
        // klog("Current time: %02d:%02d:%02d\n", time / 3600, ((time / 60) % 60), time % 60);

        clock_delay_ms(1000);
    }
}
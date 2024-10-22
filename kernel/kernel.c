#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "log.h"
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

// Boot information structure
boot_info_t boot_info;

/*
 * Main kernel entry point
 * Remember to initialize boot_info before calling this!
 */
void kmain(multiboot_info_t *mbd)
{
    // Initialize kernel logging
    klog_init();
    klog("GOOS starting...\n");

    // Initialize memory management
    mem_init(mbd);

    // Initialize interrupts
    interrupts_init();

    // Initialize drivers
    clock_init();
    kbdctl_init();

    klog("BOOTED!\n");

    for (;;)
    {
        // uint32_t time = clock_get_local();
        // klog("Current time: %02d:%02d:%02d\n", time / 3600, ((time / 60) % 60), time % 60);

        clock_delay_ms(1000);
    }
}
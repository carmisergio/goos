#include <stdbool.h>
#include <stdint.h>

#include "log.h"
#include "mem/physmem.h"
#include "boot_info.h"
#include "panic.h"
#include "boot/multiboot_structs.h"
#include "drivers/serial.h"

// Boot information structure
boot_info_t boot_info;

void alloc_test()
{
    void *mem;
    uint32_t tot = 0;
    for (int i = 0; i < 10; i++)
    {
        mem = physmem_alloc();
        if (mem != PHYSMEM_NULL)
        {
            tot += 4;
            klog("Allocated memory: %d KiB\n", tot);
        }
        else
        {
            panic("OUT_OF_MEMORY");
        }
    }
}

/*
 * Main kernel entry point
 * Remember to initialize boot_info before calling this!
 */
void kmain()
{

    serial_init(COM1);
    serial_prtstr(COM1, "HELLORLD!");

    // Initialize kernel logging
    klog_init();
    klog("GOOS starting...\n");
    klog("Hello from the higher half kernel!\n");
    klog("Kernel load addr: %x, kernel end addr: %x\n", &_kernel_start, &_kernel_end);

    // // Init physical page allocator
    // physmem_init();

    // alloc_test();
}
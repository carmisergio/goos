#include <stdbool.h>
#include <stdint.h>

#include "log.h"
#include "mem/mem.h"
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
void kmain(multiboot_info_t *mbd)
{
    // Initialize kernel logging
    klog_init();
    klog("GOOS starting...\n");

    // Initialize memory management
    mem_init(mbd);

    // Init physical page allocator
    // physmem_init();

    // alloc_test();
}
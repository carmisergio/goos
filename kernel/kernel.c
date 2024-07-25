#include <stdbool.h>
#include <stdint.h>

#include "log.h"
#include "mem/physmem.h"
#include "boot_info.h"

// Boot information structure
boot_info_t boot_info;

/*
 * Main kernel entry point
 * Remember to initialize boot_info before calling this!
 */
void kmain()
{
    // Initialize kernel logging
    klog_init();
    klog("GOOSE starting...\n");
    klog("Kernel load addr: %x, kernel end addr: %x\n", &_kernel_start, &_kernel_end);

    // Init physical page allocator
    physmem_init();
}
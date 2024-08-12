#include "mem/mem.h"

#include "log.h"
#include "boot/boot.h"
#include "boot/multiboot.h"
#include "mem/c_paging.h"
#include "mem/vmem.h"

void mem_init(multiboot_info_t *mbd)
{
    klog("Initializing memory management...\n");

    // Set current address space to bootstrap address space
    vmem_init(&bootstrap_page_dir);

    // Remove kernel lower half mapping
    vmem_unmap_range_nofree((void *)((uint32_t)&_kernel_start - KERNEL_OFFSET),
                            (uint32_t)&_kernel_end - (uint32_t)&_kernel_start);
    // NOTE: Need to cast to uint32_t because subtracting long from char * doesn't work
    // for some reason

    // Read multiboot information struct
    mb_read_data(mbd);
}
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "log.h"
#include "mem/mem.h"
#include "mem/physmem.h"
#include "mem/vmem.h"
#include "boot_info.h"
#include "panic.h"
#include "boot/multiboot_structs.h"
#include "drivers/vga.h"
#include "drivers/serial.h"

// Boot information structure
boot_info_t boot_info;

// Function prototypes
void remove_lowmem_mapping();

void alloc_test()
{
    void *allocs[1000];
    uint32_t tot = 0;
    for (int i = 0; i < 100; i++)
    {
        allocs[i] = physmem_alloc();
        if (allocs[i] != PHYSMEM_NULL)
        {
            tot += 4;
            klog("Allocated memory: %d KiB, %x\n", tot, allocs[i]);
        }
        else
        {
            panic("OUT_OF_MEMORY");
        }
        // physmem_free(mem);
    }

    for (int i = 0; i < 100; i++)
    {
        physmem_free(allocs[i]);
        klog("Freed %x\n", allocs[i]);
    }

    // Contiguous allocation test
    void *cont = physmem_alloc_n(4);
    klog("Allocated 4 pages of memory: %x\n", cont);

    tot = 0;
    for (int i = 0; i < 10000; i++)
    {
        void *mem = physmem_alloc_n(1);
        if (mem != PHYSMEM_NULL)
        {
            tot += 4;
            klog("Allocated memory: %d KiB, %x\n", tot, mem);
        }
        else
        {
            panic("OUT_OF_MEMORY");
        }
        // physmem_free(mem);
    }
}

void *alloc[30000];

void alloc_test_2()
{
    // Get one page of physical memory
    void *mem = physmem_alloc();
    klog("Physical address: %x\n", mem);

    for (int i = 0; i < 3000; i++)
    {
        // Allocate
        klog("%d\n", i);
        alloc[i] = vmem_map_range_anyk(mem, MEM_PAGE_SIZE);
    }

    for (int i = 0; i < 3000; i++)
    {
        // Deallocate
        vmem_unmap_range(alloc[i], MEM_PAGE_SIZE);
    }

    for (int i = 0; i < 3000; i++)
    {
        // Allocate
        alloc[i] = vmem_map_range_anyk(mem, MEM_PAGE_SIZE);
    }
    vmem_log_vaddrspc();

    for (int i = 0; i < 3000; i++)
    {
        // Deallocate
        vmem_unmap_range(alloc[i], MEM_PAGE_SIZE);
    }

    mem = physmem_alloc();
    klog("Physical address: %x\n", mem);

    vmem_log_vaddrspc();
}

void alloc_test_3()
{
    void *var = mem_palloc_k(1000);

    klog("Allocated: %x\n", var);

    // vmem_log_vaddrspc();

    mem_pfree(var, 1000);

    klog("Freed!");

    // vmem_log_vaddrspc();
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

    // After mem_init() cleanup
    vga_init_aftermem();
    remove_lowmem_mapping();

    // alloc_test();
    alloc_test_2();
    alloc_test_3();
}

/**
 * Remove Low memory identity mapping from the VAS
 */
void remove_lowmem_mapping()
{
    vmem_unmap_range((void *)0, 0x100000);
}
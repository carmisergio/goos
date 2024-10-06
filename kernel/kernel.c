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

#include "mem/gdt.h"

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
    for (int i = 0; i < 1000; i++)
    {

        klog("Allocating 1 page... ");
        void *addr1 = mem_palloc_k(1);
        if (addr1 != MEM_FAIL)
            klog("SUCCESS! address = %x\n", addr1);
        else
            klog("FAILURE!\n");

        klog("Allocating 1000 pages... ");
        void *addr2 = mem_palloc_k(1000);
        if (addr2 != MEM_FAIL)
            klog("SUCCESS! address = %x\n", addr2);
        else
            klog("FAILURE!\n");

        klog("Allocating 10000 pages... ");
        void *addr3 = mem_palloc_k(10000);
        if (addr3 != MEM_FAIL)
            klog("SUCCESS! address = %x\n", addr3);
        else
            klog("FAILURE!\n");

        klog("Allocating 1000 pages... ");
        void *addr4 = mem_palloc_k(1000);
        if (addr2 != MEM_FAIL)
            klog("SUCCESS! address = %x\n", addr4);
        else
            klog("FAILURE!\n");

        if (addr1 != MEM_FAIL)
            mem_pfree(addr1, 1);
        if (addr2 != MEM_FAIL)
            mem_pfree(addr2, 1000);
        if (addr3 != MEM_FAIL)
            mem_pfree(addr3, 10000);
        if (addr4 != MEM_FAIL)
            mem_pfree(addr4, 1000);
    }

    // uint32_t tot = 0;

    // for (int i = 0; i < 10000; i++)
    // {
    //     void *mem = mem_palloc_k(1);
    //     if (mem != PHYSMEM_NULL)
    //     {
    //         tot += 4;
    //         // klog("Allocated memory: %d KiB, %x\n", tot, mem);
    //     }
    //     else
    //     {
    //         panic("OUT_OF_MEMORY");
    //     }
    //     // physmem_free(mem);
    // }

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

    // alloc_test();
    // alloc_test_2();
    //
    klog("BOOTED!\n");

    alloc_test_3();
}
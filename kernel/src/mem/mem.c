#include "mem/mem.h"

#include "log.h"
#include "boot/boot.h"
#include "boot/multiboot.h"
#include "mem/const.h"
#include "mem/vmem.h"
#include "mem/physmem.h"
#include "mem/gdt.h"
#include "mem/kalloc.h"
#include "drivers/vga.h"
#include "panic.h"

void mem_init(multiboot_info_t *mbd)
{
    kdbg("[MEM] Initializing memory management...\n");

    // Set current address space to bootstrap address space
    vmem_init(&bootstrap_page_dir);

    // Make VGA driver use the VAS
    vga_init_aftermem();

    // Remove kernel lower half mapping
    vmem_unmap_range_nofree((void *)((uint32_t)&_kernel_start - KERNEL_OFFSET),
                            (uint32_t)&_kernel_end - (uint32_t)&_kernel_start);
    // NOTE: Need to cast to uint32_t because subtracting long from char * doesn't work
    // for some reason

    // Remove identity mapping for low memory
    vmem_unmap_range_nofree((void *)0, 0x100000);

    // Read multiboot information struct
    mb_read_data(mbd);

    // Initialize physical memory management
    physmem_init();

    // Free unused page tables used during boot
    vmem_purge_pagetabs();

    // Set up GDT
    setup_gdt();

    // Initialize kalloc
    kalloc_init();
}

void *mem_palloc_k(uint32_t n)
{
    // Find space in KVAS
    void *start_vaddr = vmem_palloc_k(n);

    for (uint32_t i = 0; i < n; i++)
    {
        void *vaddr = (char *)start_vaddr + i * MEM_PAGE_SIZE;

        // Allocate page of physical memory
        void *paddr = physmem_alloc();
        if (paddr == PHYSMEM_NULL)
        {
            // Free already allocated memory
            mem_pfree(start_vaddr, i);
            return MEM_FAIL;
        }

        // Map page to VAS
        if (!vmem_map(paddr, vaddr, 1))
        {
            // Free already allocated memory
            mem_pfree(start_vaddr, i + 1);
            return MEM_FAIL;
        }
    }

    return start_vaddr;
}

void mem_pfree(void *addr, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
    {
        void *vaddr = (char *)addr + i * MEM_PAGE_SIZE;

        // Get physical mapping
        void *paddr = vmem_get_phys(vaddr);
        if (paddr == PHYSMEM_NULL)
        {
            panic("MEM_PFREE_NOMAPPING", "Can't free memory that was not allocated");
            return;
        }

        // Free physical memory
        physmem_free(paddr);
    }

    // Unmap from VAS
    vmem_unmap(addr, n);
}
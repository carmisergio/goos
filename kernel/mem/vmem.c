#include "mem/vmem.h"

#include <stddef.h>
#include "string.h"

#include "log.h"
#include "panic.h"
#include "mem/const.h"
#include "mem/physmem.h"

// Internal function prototypes
static void vmem_int_set_ptes(void *paddr, void *vaddr, uint32_t n);
static void vmem_int_set_pte(void *paddr, void *vaddr);
static void vmem_int_clear_ptes(void *vaddr, uint32_t n);
static void vmem_int_clear_pte(void *vaddr);
static void *vmem_alloc_k_alreadymapped(uint32_t n);
static inline size_t vmem_int_pte_index(void *addr);
static inline size_t vmem_int_pde_index(void *addr);
static bool vmem_int_new_page_table(uint32_t n);
// static uint32_t vmem_int_find_free_pde_k();
static void vmem_int_set_pde(void *paddr, uint32_t pde_index);
static void vmem_int_clear_pde(uint32_t pde_index);
static void vmem_int_delete_unused_page_tables(uint32_t start, uint32_t n);
static bool vmem_int_is_page_table_unused(uint32_t pde);
static inline void vmem_int_flush_tlb();

// Pointer to the virtual address of the current address space's
// page directory
pde_t *cvas_pagedir;

// Pointer to the self reference mapping of the page tables
pte_t *cvas_pagetabs;

/* Public functions */

void vmem_init(pde_t *pagedir)
{
    klog("[VMEM] Initializing...\n");
    // Set current address space page directory pointer
    cvas_pagedir = pagedir;

    klog("Current VAS Page directory: %x\n", cvas_pagedir);

    // Set the self reference page table mapping pointer
    // This is where the virtual memory manager will find all the
    // currently assigned page tables
    cvas_pagetabs = (pte_t *)PAGE_DIR_SELFREF_ADDR;
}

void vmem_purge_pagetabs()
{
    vmem_int_delete_unused_page_tables(0, PDE_NUM);
}

bool vmem_map(void *paddr, void *vaddr, uint32_t n)
{
    // Iterate over all pages to map
    for (uint32_t page = 0; page < n; page++)
    {
        void *page_vaddr = (char *)vaddr + page * MEM_PAGE_SIZE;
        void *page_paddr = (char *)paddr + page * MEM_PAGE_SIZE;

        // Check if the page table for this page table entry exists,
        // otherwise create it
        uint32_t pde = vmem_int_pde_index(page_vaddr);
        if ((cvas_pagedir[pde] & PDE_FLAG_PRESENT) == 0)
        {
            // Allocate new page directory
            if (!vmem_int_new_page_table(pde))
                return false;
        }

        // Set corresponding PTE
        vmem_int_set_pte(page_paddr, page_vaddr);
    }

    return true;
}

void *vmem_map_range_anyk(void *paddr, uint32_t size)
{
    void *vaddr, *paddr_pa;
    uint32_t n_pages;

    // Page align address and size
    paddr_pa = vmem_page_aligned(paddr);

    // Calculate number of pages to map
    n_pages = vmem_n_pages_pa(paddr, size);

    // Find free range to map
    vaddr = vmem_palloc_k(n_pages);
    if (vaddr == NULL)
    {
        // kdbg("[VMEM] vmem_map_range_anyk(paddr=%x, size=%d): KVAS full\n", paddr, size);
        return NULL;
    }

    // Map pages
    vmem_map(paddr_pa, vaddr, n_pages);

    // Return pointer to address of physical mapping,
    // which could be not page aligned
    return (void *)((char *)vaddr + ((char *)paddr - (char *)paddr_pa));
}

void *vmem_map_range_anyk_noalloc(void *paddr, uint32_t size)
{
    void *vaddr, *paddr_pa;
    uint32_t n_pages;

    // Page align address and size
    paddr_pa = vmem_page_aligned(paddr);

    // Calculate number of pages to map
    n_pages = vmem_n_pages_pa(paddr, size);

    // Find free range to map
    vaddr = vmem_alloc_k_alreadymapped(n_pages);

    // If no free range is found, fail
    if (vaddr == NULL)
        return NULL;

    // Map pages
    vmem_int_set_ptes(paddr_pa, vaddr, n_pages);

    // Return pointer to address of physical mapping,
    // which could be not page aligned
    return (void *)((char *)vaddr + ((char *)paddr - (char *)paddr_pa));
}

void vmem_unmap(void *vaddr, uint32_t n)
{
    // Clear PTEs
    vmem_int_clear_ptes(vaddr, n);

    // How many PDEs does the range span?
    uint32_t pte_index = vmem_int_pte_index(vaddr);
    uint32_t pde_index = vmem_int_pde_index(vaddr);
    uint32_t pde_aligned_n = pte_index + n - (pde_index * PTE_NUM);
    uint32_t pde_span = (pde_aligned_n + PTE_NUM - 1) / (PTE_NUM);

    // Remove empty Page Tables
    vmem_int_delete_unused_page_tables(vmem_int_pde_index(vaddr), pde_span);
}

void vmem_unmap_range(void *vaddr, uint32_t size)
{
    // Page align address and size
    void *vaddr_pa = vmem_page_aligned(vaddr);

    // Calculate number of pages to map
    uint32_t n_pages = vmem_n_pages_pa(vaddr, size);

    // Unmap pages
    vmem_unmap(vaddr_pa, n_pages);
}

void vmem_unmap_range_nofree(void *vaddr, uint32_t size)
{
    // Page align address and size
    void *vaddr_pa = vmem_page_aligned(vaddr);

    // Calculate number of pages to map
    uint32_t n_pages = vmem_n_pages_pa(vaddr, size);

    // Clear respective PTEs
    vmem_int_clear_ptes(vaddr_pa, n_pages);
}

void *vmem_palloc_k(uint32_t n)
{
    uint32_t pte_index;
    uint32_t run = 0;
    void *start_addr = 0;

    // Iterate over all PDEs in the kernel VAS
    // -1 to avoid the self referencing PDE
    for (uint32_t pde = KERNEL_VAS_START / (MEM_PAGE_SIZE * PTE_NUM);
         pde < PDE_NUM - 1; pde++)
    {
        // Check if PDE is present
        if ((cvas_pagedir[pde] & PDE_FLAG_PRESENT) == 0)
        {
            // Remember address of first page in range
            if (run == 0)
                start_addr = (void *)(pde * PTE_NUM * MEM_PAGE_SIZE);

            // If PDE is present, all its addresses are free
            run += PTE_NUM;

            // If enough free pages have been found in the range,
            // return its start address
            if (run >= n)
                return start_addr;
            continue;
        }

        // Iterate over all PTEs of this PDE
        for (uint32_t pte = 0; pte < PTE_NUM; pte++)
        {
            // Offset PTE index by PDE index
            pte_index = pde * PTE_NUM + pte;

            // Check if PTE is free
            if ((cvas_pagetabs[pte_index] & PTE_FLAG_PRESENT) != 0)
            {
                // Reset run and proceed to next PTE
                run = 0;
                continue;
            }

            // Remember address of first page in range
            if (run == 0)
                start_addr = (void *)(pte_index * MEM_PAGE_SIZE);

            // Count this page in the run
            run++;

            // If enough free pages have been found in the range,
            // return its start address
            if (run >= n)
                return start_addr;
        }
    }

    // Handle the situation in which the last page of a free range is the
    // last page of the address space
    if (run >= n)
        return start_addr;

    return NULL;
}

void vmem_log_vaddrspc()
{
    klog("Current address space mappings:\n");

    // Iterate over PDEs
    for (size_t i = 0; i < PDE_NUM; i++)
    {
        // Extract PDE
        uint32_t pde = cvas_pagedir[i];

        // Check if PDE is marked as present
        if ((pde & PDE_FLAG_PRESENT) != 0)
        {
            // Iterate over PTEs
            for (size_t j = 0; j < PTE_NUM; j++)
            {
                uint32_t tabindex = i * PTE_NUM + j;

                // Check if page is present
                if ((cvas_pagetabs[tabindex] & PTE_FLAG_PRESENT) != 0)
                {
                    // Compute virtual address of entry
                    uint32_t vaddr = (i * PTE_NUM + j) * MEM_PAGE_SIZE;

                    // Get physical address of entry
                    uint32_t paddr = cvas_pagetabs[tabindex] & 0xFFFFF000;

                    klog("  - %x -> %x\n", vaddr, paddr);
                }
            }
        }
    }
}

void *vmem_get_phys(void *vaddr)
{
    // Check if corresponding PDE exists
    if ((cvas_pagedir[vmem_int_pde_index(vaddr)] & PDE_FLAG_PRESENT) == 0)
        return PHYSMEM_NULL;

    // Check if PTE is set
    uint32_t pte_index = vmem_int_pte_index(vaddr);
    if ((cvas_pagetabs[pte_index] & PTE_FLAG_PRESENT) == 0)
        return PHYSMEM_NULL;

    // Get address
    return (void *)(cvas_pagetabs[pte_index] & PTE_ADDR_MASK);
}

/* Internal functions */

/*
 * Find range of at least n free pages in the page tables already mapped
 * in the page directory for the KVAS
 * #### Parameters:
 *   - uint32_t n: number of free pages required
 * #### Returns:
 *   - void *: pointer to the first page of the range (page aligned),
 *       null on failure
 * #### Fails:
 *     This function fails when no free range big enough is found
 */
static void *vmem_alloc_k_alreadymapped(uint32_t n)
{
    uint32_t pte_index;
    uint32_t run = 0;
    void *start_addr = 0;

    // Iterate over all PDEs in the kernel VAS
    // -1 to avoid the self referencing PDE
    for (uint32_t pde = KERNEL_VAS_START / (MEM_PAGE_SIZE * PTE_NUM);
         pde < PDE_NUM - 1; pde++)
    {
        // Check if PDE is present
        if ((cvas_pagedir[pde] & PDE_FLAG_PRESENT) == 0)
        {
            run = 0;
            continue;
        }

        // Iterate over all PTEs of this PDE
        for (uint32_t pte = 0; pte < PTE_NUM; pte++)
        {
            // Offset PTE index by PDE index
            pte_index = pde * PTE_NUM + pte;

            // Check if PTE is free
            if ((cvas_pagetabs[pte_index] & PTE_FLAG_PRESENT) != 0)
            {
                // Reset run and proceed to next PTE
                run = 0;
                continue;
            }

            // Remember address of first page in range
            if (run == 0)
                start_addr = (void *)(pte_index * MEM_PAGE_SIZE);

            // Count this page in the run
            run++;

            // If enough free pages have been found in the range,
            // return its start address
            if (run >= n)
                return start_addr;
        }
    }

    // Handle the situation in which the last page of a free range is the
    // last page of the address space
    if (run >= n)
        return start_addr;

    return NULL;
}

/*
 * Create PTE for some contiguous pages
 * #### Parameters:
 *  - void *paddr: physical address of the first page (page aligned)
 *  - void *vaddr: virtual address of the first page (page aligned)
 *  - uint32_t n: number of pages to map
 * #### Notes:
 *    This function panics if mapping already mapped pages and pages
 *    for which a page table is not assigned in the page directory
 */
static void vmem_int_set_ptes(void *paddr, void *vaddr, uint32_t n)
{
    // Cast void * to pointer of known size
    char *ppage = (char *)paddr;
    char *vpage = (char *)vaddr;

    // Iterate over starting address of all pages in range
    for (uint32_t i = 0; i < n; i++)
    {
        vmem_int_set_pte(ppage, vpage);

        // Next page address
        vpage += MEM_PAGE_SIZE;
        ppage += MEM_PAGE_SIZE;
    }
}

/*
 * Create PTE for one page
 * #### Parameters:
 *  - void *paddr: physical address of the page (page aligned)
 *  - void *vaddr: virtual address of the page (page aligned)
 * #### Notes:
 *    This function panics if mapping already mapped pages and pages
 *    for which a page table is not assigned in the page directory
 */
static void vmem_int_set_pte(void *paddr, void *vaddr)
{
    size_t pte_index;
    uint32_t pte;

    // If no matching PDE is found for this page, there is no
    // page table to modify
    if ((cvas_pagedir[vmem_int_pde_index(vaddr)] & PDE_FLAG_PRESENT) == 0)
        panic("VMEM_INT_MAP_PTE_PDE_NOT_PRESENT", "Trying to set PTE in a non-existent Page Table");

    // Compute PTE index
    pte_index = vmem_int_pte_index(vaddr);

    // Check if page is already mapped
    if ((cvas_pagetabs[pte_index] & PTE_FLAG_PRESENT) != 0)
        panic("VMEM_INT_MAP_PTE_ALREADY_MAPPED", "Trying to set PTE, but already set");

    // Set PTE
    pte = (uint32_t)paddr;
    pte |= PTE_FLAG_PRESENT;
    cvas_pagetabs[pte_index] = pte;
}

/*
 * Remove mapping from VAS for some pages
 * #### Parameters:
 *  - void *vaddr: virtual address of the first page (page aligned)
 *  - uint32_t n: number of pages to unmap
 * #### Notes:
 *    This function panics if unmapping already unmapped pages and pages
 *    for which a page table is not assigned in the page directory
 */
static void vmem_int_clear_ptes(void *vaddr, uint32_t n)
{
    // Cast void * to pointer of known size
    char *page = (char *)vaddr;

    // Iterate over starting address of all pages in range
    for (uint32_t i = 0; i < n; i++)
    {
        vmem_int_clear_pte(page);

        // Next page address
        page += MEM_PAGE_SIZE;
    }
}

/*
 * Remove mapping from VAS for one page
 * #### Parameters:
 *  - void *vaddr: virtual address of the page (page aligned)
 * #### Notes:
 *    This function panics if unmapping already unmapped pages and pages
 *    for which a page table is not assigned in the page directory
 */
static void vmem_int_clear_pte(void *vaddr)
{
    size_t pte_index;
    // If no matching PDE is found for this page, there is no
    // page table to modify
    if ((cvas_pagedir[vmem_int_pde_index(vaddr)] & PDE_FLAG_PRESENT) == 0)
        panic("VMEM_INT_UNMAP_PDE_NOT_PRESENT", "Trying to unmap from a non-existent Page Table");

    // Compute PTE index
    pte_index = vmem_int_pte_index(vaddr);

    // Check if page is already unmapped
    if ((cvas_pagetabs[pte_index] & PTE_FLAG_PRESENT) == 0)
        panic("VMEM_INT_UNMAP_PTE_NOT_PRESENT", "Trying to unmap, but no mapping is present");

    // Clear PTE
    cvas_pagetabs[pte_index] = 0;
}

/*
 * Calculate index into the page directory self-mapping space,
 * to be able to access the page table entry of any virtual memory
 * page.
 * #### Parameters:
 *  - void *vaddr: virtual address of the wanted page (page aligned)
 * #### Returns:
 *    size_t: index in the self-mappping area
 */
static inline size_t vmem_int_pte_index(void *addr)
{
    return (size_t)addr / MEM_PAGE_SIZE;
}

/*
 * Calculate index of the PDE corresponding to a virtual memory page
 * #### Parameters:
 *  - void *vaddr: virtual address of the page (page aligned)
 * #### Returns:
 *    size_t: PDE index
 */
static inline size_t vmem_int_pde_index(void *addr)
{
    return (size_t)addr / (MEM_PAGE_SIZE * PDE_NUM);
}

/*
 * Allocate a new Page Table and map it to the specified PDE
 * #### Parameters:
 *   - uint32_t pde: PDE index to populate
 * #### Returns:
 *    bool: true if succesful
 * #### Fails:
 *     This function fails when there isn't a free entry in the Page Directory
 */
static bool vmem_int_new_page_table(uint32_t pde)
{
    // Allocate new page of physical memory
    void *page;
    if ((page = physmem_alloc()) == PHYSMEM_NULL)
    {
        // kdbg("[VMEM] vmem_int_new_page_table(pde=%d): physmem_alloc() failed\n", pde);
        return false;
    }

    // Set PDE
    vmem_int_set_pde(page, pde);

    // Clear Page Table
    memset((void *)(cvas_pagetabs + pde * PTE_NUM), 0x00, sizeof(pte_t) * PTE_NUM);

    // kdbg("[VMEM] Allocated new page table (PDE=%d, phys addr=%x)\n", pde, page);

    return true;
}

/*
 * Find a free PDE in the current Page Directory
 * #### Returns:
 *   - uint32_t: index into the page directory, 0 on failure
 * #### Fails:
 *     This function fails when there isn't a free PDE in the current page directory
 */
// static uint32_t vmem_int_find_free_pde_k()
// {
//     // Iterate over all PDEs in the kernel VAS
//     for (uint32_t pde = KERNEL_VAS_START / (MEM_PAGE_SIZE * PTE_NUM);
//          pde < PDE_NUM - 1; pde++)
//     {
//         // Check if PDE is free
//         if ((cvas_pagedir[pde] & PDE_FLAG_PRESENT) == 0)
//             return pde;
//     }

//     // No free space found
//     return 0;
// }

/*
 * Create PDE for one Page Table
 * #### Parameters:
 *  - void *paddr: physical address of the new Page Table
 *  - uint32_t pde_index: index into the Page Directory
 */
static void vmem_int_set_pde(void *paddr, uint32_t pde_index)
{
    uint32_t pde;

    // Check if page is already mapped
    if ((cvas_pagedir[pde_index] & PDE_FLAG_PRESENT) != 0)
        panic("VMEM_INT_SET_PDE_ALREADY_SET", "Trying to set PDE, but already set");

    // Set PDE
    pde = (uint32_t)paddr;
    pde |= PDE_FLAG_PRESENT;
    cvas_pagedir[pde_index] = pde;
}

/*
 * Clear given Page Directory Entry
 * #### Parameters:
 *  - uint32_t pde_index: index into the Page Directory
 */
static void vmem_int_clear_pde(uint32_t pde_index)
{

    // Check if page is already unmapped
    if ((cvas_pagedir[pde_index] & PDE_FLAG_PRESENT) == 0)
        panic("VMEM_INT_CLEAR_PDE_NOT_PRESENT", "Trying to clear PDE, but already clear");

    // Clear PTE
    cvas_pagedir[pde_index] = 0;
}

/*
 * Deletes PDEs and deallocates unused page tables in range of page directory entries
 * #### Parameters:
 *  - uint32_t start: starting page directory entry index
 *  - uint32_t n: number of pdes to check
 */
static void vmem_int_delete_unused_page_tables(uint32_t start, uint32_t n)
{
    // Iterate over PDEs
    for (uint32_t pde = start; pde < start + n; pde++)
    {
        // Check if the PDE is present
        if ((cvas_pagedir[pde] & PDE_FLAG_PRESENT) == 0)
            continue;

        // Check if the Page table is unused
        if (vmem_int_is_page_table_unused(pde))
        {
            // Get physical address of page table
            void *phys_page = (void *)(cvas_pagedir[pde] & PDE_ADDR_MASK);
            physmem_free(phys_page);

            // Clear PDE
            vmem_int_clear_pde(pde);

            // kdbg("[VMEM] Freed page table (PDE=%d, phys addr=%x)\n", pde, phys_page);
        }
    }

    vmem_int_flush_tlb();
}

/*
 * Check if the page table associated with a given PDE is unused
 * #### Parameters:
 *  - uint32_t pde: PDE index
 * #### Returns:
 *   bool: true if the Page Table is unused
 */
static bool vmem_int_is_page_table_unused(uint32_t pde)
{
    // Iterate over all PTEs of thie PDE
    for (uint32_t pte_index = pde * PTE_NUM; pte_index < (pde + 1) * (PTE_NUM); pte_index++)
    {
        // If any PTE is present, the Page Table is in use
        if ((cvas_pagetabs[pte_index] & PTE_FLAG_PRESENT) != 0)
            return false;
    }

    return true;
}

/**
 * Flush TLBs
 */
static inline void vmem_int_flush_tlb()
{
    uint32_t tmp;

    // Flush TLB by writing to the CR3 register
    asm(
        "mov %%cr3, %0\n"
        "mov %0, %%cr3\n" : [temp] "=r"(tmp) :);
}
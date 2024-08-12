#include "mem/vmem.h"

#include <stddef.h>

#include "log.h"
#include "panic.h"
#include "mem/c_paging.h"

// Internal function prototypes
static void vmem_int_set_ptes(void *paddr, void *vaddr, uint32_t n);
static void vmem_int_set_pte(void *paddr, void *vaddr);
static void vmem_int_clear_ptes(void *vaddr, uint32_t n);
static void vmem_int_clear_pte(void *vaddr);
static inline size_t vmem_int_pte_index(void *addr);
static inline size_t vmem_int_pde_index(void *addr);
static void *vmem_int_find_free_pages_k(uint32_t n);

// Pointer to the virtual address of the current address space's
// page directory
pde_t *cvas_pagedir;

// Pointer to the self reference mapping of the page tables
pte_t *cvas_pagetabs;

/* Public functions */

void vmem_init(pde_t *pagedir)
{
    // Set current address space page directory pointer
    cvas_pagedir = pagedir;

    // Set the self reference page table mapping pointer
    // This is where the virtual memory manager will find all the
    // currently assigned page tables
    cvas_pagetabs = (pte_t *)PAGE_DIR_SELFREF_ADDR;
}

void *vmem_map_range_anyk_noalloc(void *paddr, uint32_t size)
{
    void *vaddr, *paddr_pa;
    uint32_t size_pa, n_pages;

    // Page align address and size
    paddr_pa = vmem_page_aligned(paddr);
    size_pa = (uint32_t)paddr + size - (uint32_t)paddr_pa;

    // Calculate number of pages to map
    n_pages = vmem_n_pages(size_pa);

    // Find free range to map
    vaddr = vmem_int_find_free_pages_k(n_pages);

    // If no free range is found, fail
    if (vaddr == NULL)
        return NULL;

    // Map pages
    vmem_int_set_ptes(paddr_pa, vaddr, n_pages);

    // Return pointer to address of physical mapping,
    // which could be not page aligned
    return vaddr + (paddr - paddr_pa);
}

void vmem_unmap_range_nofree(void *vaddr, uint32_t size)
{
    // Clear respective PTEs
    vmem_int_clear_ptes(vaddr, vmem_n_pages(size));
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
                    uint32_t paddr = cvas_pagetabs[tabindex] & 0x3FF000;

                    klog("  - %x -> %x\n", vaddr, paddr);
                }
            }
        }
    }
}

/* Internal functions */

/*
 * Create PTE for some contiguous pages
 * #### Arguments:
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
 * #### Arguments:
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
        panic("VMEM_INT_MAP_PTE_PDE_NOT_PRESENT");

    // Compute PTE index
    pte_index = vmem_int_pte_index(vaddr);

    // Check if page is already mapped
    if ((cvas_pagetabs[pte_index] & PTE_FLAG_PRESENT) != 0)
        panic("VMEM_INT_MAP_PTE_ALREADY_MAPPED");

    // Set PTE
    pte = (uint32_t)paddr;
    pte |= PTE_FLAG_PRESENT;
    cvas_pagetabs[pte_index] = pte;
}

/*
 * Remove mapping from VAS for some pages
 * #### Arguments:
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
 * #### Arguments:
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
        panic("VMEM_INT_UNMAP_PDE_NOT_PRESENT");

    // Compute PTE index
    pte_index = vmem_int_pte_index(vaddr);

    // Check if page is already unmapped
    if ((cvas_pagetabs[pte_index] & PTE_FLAG_PRESENT) == 0)
        panic("VMEM_INT_UNMAP_PTE_NOT_PRESENT");

    // Clear PTE
    cvas_pagetabs[pte_index] = 0;
}

/*
 * Calculate index into the page directory self-mapping space,
 * to be able to access the page table entry of any virtual memory
 * page.
 * #### Arguments:
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
 * #### Arguments:
 *  - void *vaddr: virtual address of the page (page aligned)
 * #### Returns:
 *    size_t: PDE index
 */
static inline size_t vmem_int_pde_index(void *addr)
{
    return (size_t)addr / (MEM_PAGE_SIZE * PDE_NUM);
}

/*
 * Find range of at least n free pages in the page tables already mapped
 * in the page directory for the KVAS
 * #### Arguments:
 *   - uint32_t n: number of free pages required
 * #### Returns:
 *   - void *: pointer to the first page of the range (page aligned),
 *       null on failure
 * #### Fails:
 *     This function fails when no free range big enough is found
 */
static void *vmem_int_find_free_pages_k(uint32_t n)
{
    uint32_t pte_index;
    uint32_t run = 0;
    void *start_addr = 0;

    // Iterate over all PDEs in the kernel VAS
    for (uint32_t pde = KERNEL_VAS_START / (MEM_PAGE_SIZE * PTE_NUM);
         pde < PDE_NUM; pde++)
    {
        // Check if PDE is present
        if ((cvas_pagedir[pde] & PDE_FLAG_PRESENT) == 0)
        {
            // Reset run and proceed to next PDE
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
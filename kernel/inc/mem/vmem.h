#ifndef _MEM_VMEM_H
#define _MEM_VMEM_H 1

#include <stdint.h>
#include <stdbool.h>

#include "mem/const.h"

// Page Directory Entry type
typedef uint32_t pde_t;

// Page Table Entry type
typedef uint32_t pte_t;

/*
 * Sets the current address space's page directory location
 * #### Parameters:
 *   - pde_t *pd: page directory virtual address
 */
void vmem_init(pde_t *pd);

/*
 * Unmaps and frees unused page tables
 */
void vmem_purge_pagetabs();

/*
 * Maps contiguous pages from a physical address to a virtual address,
 * allocating new page tables if needed
 * #### Parameters:
 *  - void *paddr: physical address of first page (page aligned)
 *  - void *vaddr: virtual address of first page (page aligned)
 *  - uint32_t n: number of pages
 * #### Returns:
 *    bool: true if succesful
 * #### Notes:
 *    this function fails if page table allocation fails
 */
bool vmem_map(void *paddr, void *vaddr, uint32_t n);

/*
 * Maps contiguous pages from a physical address to a virtual address,
 * but doesn't allocate new page tables
 * #### Parameters:
 *  - void *paddr: physical address of first page (page aligned)
 *  - void *vaddr: virtual address of first page (page aligned)
 *  - uint32_t n: number of pages
 * #### Returns:
 *    bool: true if succesful
 * #### Notes:
 *    this function fails if one of the required page table is not
 *    mapped
 */
// bool vmem_map_noalloc(void *paddr, void *vaddr, uint32_t n);

/*
 * Maps contiguous pages from a physical to an available space
 * in the kernel address space, allocating new page tables
 * if needed
 * #### Parameters:
 *  - void *paddr: physical address of first page (page aligned)
 *  - uint32_t n: number of pages
 * #### Returns:
 *    void *: pointer to the virtual address of the mapping,
 *        NULL if the mapping failed
 * #### Notes:
 *    this function fails if there isn't enough space in the kernel address
 *    space or if the allocation of a new page table fails
 */
// void *vmem_map_anyk(void *paddr, uint32_t n);

/*
 * Maps contiguous pages from a physical to an available space
 * in the kernel address space, but doesn't allocate new page
 * tables
 * #### Parameters:
 *  - void *paddr: physical address of first page (page aligned)
 *  - uint32_t n: number of pages
 * #### Returns:
 *    void *: pointer to the virtual address of the mapping,
 *        NULL if the mapping failed
 * #### Notes:
 *    this function fails if there isn't enough space in the kernel
 *    address space to which page tables are already assigned
 */
// void *vmem_map_anyk_noalloc(void *paddr, uint32_t n);

/*
 * Maps a physical address range into the kernel address space,
 * mapping all pages the range spans. Allocates new page tables
 * if necessary.
 * #### Parameters:
 *  - void *paddr: start physical address
 *  - uint32_t size: size of range
 * #### Returns:
 *    void *: pointer to the virtual address of the mapping,
 *        NULL if the mapping failed
 * #### Notes:
 *    this function fails if there isn't enough space in the kernel address
 *    space or if the allocation of a new page table fails
 */
void *vmem_map_range_anyk(void *paddr, uint32_t size);

/*
 * Maps a physical address range into the kernel address space,
 * mapping all pages the range spans. Doen't allocate new page
 * tables.
 * #### Parameters:
 *  - void *paddr: start physical address
 *  - uint32_t size: size of range
 * #### Returns:
 *    void *: pointer to the virtual address of the mapping,
 *        NULL if the mapping failed
 * #### Notes:
 *    this function fails if there isn't enough space in the page ables
 *    already assigned to the kernel VAS
 */
void *vmem_map_range_anyk_noalloc(void *paddr, uint32_t size);

/*
 * Unmaps pages from the current virtual address space,
 * and frees any page tables left empty
 * #### Parameters:
 *   - void *vaddr: first page virtual address (page aligned)
 *   - uint32_t n: number of pages
 */
void vmem_unmap(void *vaddr, uint32_t n);

/*
 * Unmaps pages from the current virtual address space,
 * but doesn't free any page tables that remain empty
 * #### Parameters:
 *   - void *vaddr: first page virtual address (page aligned)
 *   - uint32_t n: number of pages
 */
// void vmem_unmap_nofree(void *vaddr, uint32_t n);

/*
 * Unmaps address range from the current VAS,
 * freeing page tables left empty
 * #### Parameters:
 *   - void *vaddr: start virtual address
 *   - uint32_t size: size of address range
 * #### Notes:
 *     The address supplied to this function does not need to be
 *     page aligned. This function unmaps all pages that the
 *     address range spans
 */
void vmem_unmap_range(void *vaddr, uint32_t size);

/*
 * Unmaps address range from the current VAS,
 * but doesn't free any page tables that remain empty
 * #### Parameters:
 *   - void *vaddr: start virtual address
 *   - uint32_t size: size of address range
 * #### Notes:
 *     The address supplied to this function does not need to be
 *     page aligned. This function unmaps all pages that the
 *     address range spans
 */
void vmem_unmap_range_nofree(void *vaddr, uint32_t size);

/*
 * Find range of at least n free pages in the KVAS
 * #### Parameters:
 *   - uint32_t n: number of free pages required
 * #### Returns:
 *   - void *: pointer to the first page of the range (page aligned),
 *       null on failure
 * #### Fails:
 *     This function fails when no free range big enough is found
 */
void *vmem_palloc_k(uint32_t n);

/*
 * Get physical mapping of a page
 * #### Parameters:
 *   - void* vaddr: virtual address (page aligned)
 * #### Returns:
 *     void* physical address, PHYSMEM_NULL if there is no mapping
 */
void *vmem_get_phys(void *vaddr);

/*
 * Free all pages in the current User Virtual Address Space
 */
void vmem_destroy_uvas();

/*
 * Create a new virtual address space
 * Returns a pointer to the new page directory
 */
pde_t *vmem_new_vas();

/*
 * Delete a virtual address space's page table
 */
void vmem_delete_vas(void *pagedir);

/*
 * Switch to a new virtual address space
 * #### Parameters
 *   - pagedir: pointer to the new page directory
 */
void vmem_switch_vas(pde_t *pagedir);

/*
 * Get a pointer to the current address space's page directory
 */
pde_t *vmem_cur_vas();

// Check if pointer is a valid userspace pointer
// (doesn't cross into the KVAS)
bool vmem_check_user_ptr(void *ptr, uint32_t size);

/*
 * Calculate number of pages from address range size
 * #### Parameters:
 *   - uint32_t size: address range size
 * #### Returns:
 *     uint32_t: number of pages
 */
static inline uint32_t vmem_n_pages(uint32_t size)
{
    return (size + MEM_PAGE_SIZE - 1) / MEM_PAGE_SIZE;
}

/*
 * Page align address
 * #### Parameters:
 *   - void *addr: address
 * #### Returns:
 *     void *: page aligned address
 */
static inline void *vmem_page_aligned(void *addr)
{
    return (void *)((((uint32_t)addr) / MEM_PAGE_SIZE) * MEM_PAGE_SIZE);
}

/*
 * Calculate number of pages spanned by a not necessarily
 *  page aligned range
 * #### Parameters:
 *   - void* addr: address range start
 *   - uint32_t size: size of the range
 * #### Returns:
 *     uint32_t: number of pages
 */
static inline uint32_t vmem_n_pages_pa(void *addr, uint32_t size)
{
    // Page align address and size
    uint32_t paddr_pa = (uint32_t)vmem_page_aligned(addr);
    uint32_t size_pa = (uint32_t)addr + size - (uint32_t)paddr_pa;

    // Calculate number of pages
    return vmem_n_pages(size_pa);
}

// Log virtual address space mappings
void vmem_log_vaddrspc();
void vmem_log_pagedir();

#endif
#ifndef _MEM_VMEM_H
#define _MEM_VMEM_H 1

#include <stdint.h>
#include <stdbool.h>

#include "mem/c_paging.h"

#ifdef __cplusplus
extern "C"
{
#endif

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
    // bool vmem_map(void *paddr, void *vaddr, uint32_t n);

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
    // void vmem_unmap(void *vaddr, uint32_t n);

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

    // Log virtual address space mappings
    void vmem_log_vaddrspc();

    // Switch address space

#ifdef __cplusplus
}
#endif

#endif
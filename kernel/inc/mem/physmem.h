#ifndef _PHYSMEM_H
#define _PHYSMEM_H 1

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define PHYSMEM_NULL (void *)UINT32_MAX

    /*
     * Initialize physical memory page allocator
     */
    void physmem_init();

    /*
     * Allocate a new physical memory page
     * #### Returns:
     *     void *: physical address of the page or
     *             PHYSMEM_NULL (0xFFFFFFFF) on failure
     */
    void *physmem_alloc();

    /*
     * Allocate n pages of contiguous physical memory
     * #### Parameters:
     *   - uint32_t n: number of pages
     * #### Returns:
     *     void *: physical address of the the first page or
     *             PHYSMEM_NULL (0xFFFFFFFF) on failure
     */
    void *physmem_alloc_n(uint32_t n);

    /*
     * Free a new physical memory page
     * #### Parameters:
     *   - void * addr: physical address of page
     */
    void physmem_free(void *addr);

    /*
     * Free n pages of physical memory
     * #### Parameters:
     *   - void * addr: physical address of page
     *   - uint32_t n: number of pages
     */
    void physmem_free_n(void *addr, uint32_t n);
    /*
     * Check if a physical memory page is free
     * #### Parameters:
     *   - void * addr: physical address of page
     * #### Returns:
     *   bool: true if page is free
     */
    bool physmem_is_free(void *addr);

    /*
     * Allocate n pages of contiguous physical memory
     * for use as an ISA DMA buffer
     * The memory allocated by this function MUST be below 16M
     * and not cross a 64K boundary
     * #### Returns:
     *     void *: physical address of the page or
     *             PHYSMEM_NULL (0xFFFFFFFF) on failure
     */
    void *physmem_alloc_isadma(uint32_t n);

#ifdef __cplusplus
}
#endif

#endif
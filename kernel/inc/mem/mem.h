#ifndef _MEM_MEM_H
#define _MEM_MEM_H 1

#include <stdint.h>
#include <stdbool.h>

#include "boot/multiboot_structs.h"

#define MEM_FAIL (void *)UINT32_MAX

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * Initialize memory management
     * #### Parameters:
     *   - multiboot_info_t *mbd: physical pointer to the multiboot info struct
     */
    void mem_init(multiboot_info_t *mbd);

    /*
     * Allocate n pages of memory in the KVAS
     * NOTE: the allocated memory is contiguous in the VAS
     * but not in the physical address space
     * #### Parameters:
     *   - uint32_t n: number of pages
     * #### Returns:
     *   void *: virtual address of the first allocaed page,
     *           MEM_FAIL on failure
     * #### Fails
     *   This function fails on physical memory allocation
     *   and VAS mapping errors
     *
     */
    void *mem_palloc_k(uint32_t n);

    /*
     * Deallocate n pages from the VAS
     * #### Parameters:
     *   - void * addr: virtual address of the first page
     *   - uint32_t n: number of pages
     * #### Panics:
     *     This function panics if there was no physical memory mapped to the
     *     specified virtual address
     */
    void mem_pfree(void *addr, uint32_t n);

    /*
     * Make a page of virtual memory available for user
     * DOESN't fail if a page is already mapped
     * #### Parameters:
     *   - void *vaddr: virtual address (page aligned)
     *   - uint32_t n: number of pages
     * #### Returns:
     *    false on failure
     */
    bool mem_make_avail(void *vaddr, uint32_t n);

#ifdef __cplusplus
}
#endif

#endif
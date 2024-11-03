#ifndef _MEM_KALLOC_H
#define _MEM_KALLOC_H 1

#include <stddef.h>

/*
 * Granular memory allocator for kernel use
 */

/*
 * Initialize kalloc
 */
void kalloc_init();

/*
 * Allocate at least N bytes of memory in the kernel VAS
 * #### Parameters:
 *   - n: number of bytes needed
 * #### Returns:
 *   Pointer to the allocated memory, NULL on error
 */
void *kalloc(size_t n);

/*
 * Free memory allocated with kalloc()
 * #### Parameters:
 *   - void *ptr: pointer to the memory to deallocate.
 *          NOTE: must have been allocated with kalloc()
 */
void kfree(void *ptr);

// Print memory chains
void kalloc_dbg_block_chain();

#endif
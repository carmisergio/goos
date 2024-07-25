#ifndef _PHYSMEM_H
#define _PHYSMEM_H 1

#include "multiboot_structs.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * Initialize physical memory page allocator
     *
     * Params:
     *     multiboot_info_t *mbd: pointer to the multiboot info structure
     * Returns: void
     */
    void physmem_init();

#ifdef __cplusplus
}
#endif

#endif
#ifndef _MEM_MEM_H
#define _MEM_MEM_H 1

#include "boot/multiboot_structs.h"

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

#ifdef __cplusplus
}
#endif

#endif
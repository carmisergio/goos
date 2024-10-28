#ifndef _BOOT_MULTIBOOT_H
#define _BOOT_MULTIBOOT_H 1

#include "boot/multiboot_structs.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * Read multiboot data passed by the bootloader and store it in kernel-managed memory
     * #### Parameters:
     *   - multiboot_info_t *mbd: physical pointer to the multiboot info struct
     */
    void mb_read_data(multiboot_info_t *mbd);

#ifdef __cplusplus
}
#endif

#endif
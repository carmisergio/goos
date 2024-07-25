/*
 *  Boot info structure
 *  Contains information passed to the kernel by the bootloader
 */

#ifndef _BOOT_INFO_H
#define _BOOT_INFO_H 1

#include <stdint.h>

#define BOOT_INFO_PHYSMMAP_MAX_ENTRIES 32

// Entry in the physical memory map of the boot_info struct
struct physmmap_entry
{
    uint32_t addr;   // Starting address (page aligned)
    uint32_t npages; // Number of pages
};
typedef struct physmmap_entry physmmap_entry_t;

// Boot info struct
struct boot_info
{
    // Physical memory map
    physmmap_entry_t physmmap[BOOT_INFO_PHYSMMAP_MAX_ENTRIES];
    uint32_t physmmap_n;
};
typedef struct boot_info boot_info_t;

// Boot info struct
extern boot_info_t boot_info;

// Kernel load information
extern void *_kernel_start;
extern void *_kernel_end;

#endif
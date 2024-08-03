/*
 * This file contains initialization code necessary when the kernel is booting from multiboot
 */

#include <stdbool.h>
#include <stdint.h>

#include "boot/multiboot_structs.h"
#include "boot_info.h"
#include "mem/c_paging.h"
#include "kernel.h"

#include "drivers/serial.h"

// Pointer to the physical address of the boot info struct
boot_info_t *boot_info_phys = (boot_info_t *)((void *)&boot_info - KERNEL_OFFSET);

// Internal function prototypes
static void _mb_setup_boot_info(multiboot_info_t *mbd);
static void _mb_setup_boot_info_physmmap(multiboot_memory_map_t *mmap, uint32_t n);
static void _mb_add_physmmap_entry(uint32_t start, uint32_t size);

/**
 * Multiboot initialization entry point
 */
int multiboot_init(multiboot_info_t *mbd, uint32_t magic)
{
    // Check bootloader identification, exit if wront
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
        return 1;

    // Check if memory map is present in multiboot info struct
    if (!(mbd->flags & MULTIBOOT_INFO_MEM_MAP))
        return 1;

    // Set up boot_info struct from multiboot info struct
    _mb_setup_boot_info(mbd);

    return 0;
}

/*
 * Set up boot_info struct from the multiboot_info struct
 * provided by the bootloader
 */
static void _mb_setup_boot_info(multiboot_info_t *mbd)
{
    // Set up physmmap
    _mb_setup_boot_info_physmmap((multiboot_memory_map_t *)mbd->mmap_addr,
                                 mbd->mmap_length);
}

/*
 * Set up physmmap in boot info from multiboot memory map
 */
static void _mb_setup_boot_info_physmmap(multiboot_memory_map_t *mmap, uint32_t len)
{
    // Initialize physmmap
    boot_info_phys->physmmap_n = 0;

    // Calc number of entries
    uint32_t n = len / sizeof(multiboot_memory_map_t);

    // Iterate over multiboot memory map entries
    for (uint32_t i = 0; i < n; i++)
    {
        // Check if memory is available
        // And if its address is below the 32-bit boundary
        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE &&
            mmap[i].addr < UINT32_MAX)
        {
            // Maximum physmmap entries exceeded
            if (boot_info_phys->physmmap_n >= BOOT_INFO_PHYSMMAP_MAX_ENTRIES)
                break;

            // Truncate pages that go above 32-bit boundary
            uint64_t len = mmap[i].len;
            if (mmap[i].addr + len > UINT32_MAX)
                len = UINT32_MAX - mmap[i].addr;

            _mb_add_physmmap_entry(mmap[i].addr, len);
        }
    }
}

/*
 * Add entry to the boot_info struct physmmap,
 * page aligning all start and end addresses
 */
static void _mb_add_physmmap_entry(uint32_t start, uint32_t size)
{
    // Construct entry
    physmmap_entry_t new_entry;
    new_entry.addr =
        ((start + MEM_PAGE_SIZE - 1) / MEM_PAGE_SIZE) * MEM_PAGE_SIZE;
    new_entry.npages =
        ((start + size) - (new_entry.addr)) / MEM_PAGE_SIZE;
    // NOTE: page align starting address

    // Add it to phys_mmap
    boot_info_phys->physmmap[boot_info.physmmap_n] = new_entry;
    boot_info_phys->physmmap_n++;
}

int test_func()
{
    return 0;
}
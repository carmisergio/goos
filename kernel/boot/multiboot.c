#include "boot/multiboot.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "log.h"
#include "boot/multiboot_structs.h"
#include "boot_info.h"
#include "mem/const.h"
#include "mem/vmem.h"
#include "panic.h"

// Internal function prototypes
static void mb_setup_boot_info(multiboot_info_t *mbd);
static void mb_setup_boot_info_physmmap(multiboot_memory_map_t *mmap, uint32_t n);
static void mb_add_physmmap_entry(uint32_t start, uint32_t size);

/* Public functions */

void mb_read_data(multiboot_info_t *mbd_phys)
{
    void *mmap_addr;
    uint32_t mmap_length;
    multiboot_info_t *mbd;
    multiboot_memory_map_t *mmap;

    klog("Reading multiboot data...\n");

    // Map multiboot struct in kernel virtual memory
    mbd = vmem_map_range_anyk_noalloc(
        mbd_phys, sizeof(multiboot_info_t));

    // NOTE: Here we can read the multiboot info struct as it is now
    //       mapped into the kVAS

    // Check if memory map is present in multiboot info struct
    if (!(mbd->flags & MULTIBOOT_INFO_MEM_MAP))
        panic("MB_READ_DATA_NO_MEMMAP", "No memory map in Multiboot info structure");

    // Read memory map information
    mmap_addr = (void *)mbd->mmap_addr;
    mmap_length = mbd->mmap_length;

    // Unmap multiboot info struct from kVAS
    vmem_unmap_range_nofree(mbd, sizeof(multiboot_info_t));

    // Map multiboot memory map into kVAS
    mmap = vmem_map_range_anyk_noalloc(
        mmap_addr, sizeof(multiboot_memory_map_t) * mmap_length);

    // Read data from multiboot memory map
    mb_setup_boot_info_physmmap(mmap, mmap_length);

    // Unmap multiboot memory map
    vmem_unmap_range_nofree(mbd, sizeof(multiboot_info_t));
}

/* Internal functions */

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
    mb_setup_boot_info(mbd);

    return 0;
}

/*
 * Set up boot_info struct from the multiboot_info struct
 * provided by the bootloader
 */
static void mb_setup_boot_info(multiboot_info_t *mbd)
{
    // Set up physmmap
    mb_setup_boot_info_physmmap((multiboot_memory_map_t *)mbd->mmap_addr,
                                mbd->mmap_length);
}

/*
 * Set up physmmap in boot info from multiboot memory map
 */
static void mb_setup_boot_info_physmmap(multiboot_memory_map_t *mmap, uint32_t len)
{
    // Initialize physmmap
    boot_info.physmmap_n = 0;

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
            if (boot_info.physmmap_n >= BOOT_INFO_PHYSMMAP_MAX_ENTRIES)
                break;

            // Truncate pages that go above 32-bit boundary
            uint64_t len = mmap[i].len;
            if (mmap[i].addr + len > UINT32_MAX)
                len = UINT32_MAX - mmap[i].addr;

            mb_add_physmmap_entry(mmap[i].addr, len);
        }
    }
}

/*
 * Add entry to the boot_info struct physmmap,
 * page aligning all start and end addresses
 */
static void mb_add_physmmap_entry(uint32_t start, uint32_t size)
{
    // Construct entry
    physmmap_entry_t new_entry;
    new_entry.addr =
        ((start + MEM_PAGE_SIZE - 1) / MEM_PAGE_SIZE) * MEM_PAGE_SIZE;
    new_entry.npages =
        ((start + size) - (new_entry.addr)) / MEM_PAGE_SIZE;
    // NOTE: page align starting address

    // Add it to phys_mmap
    boot_info.physmmap[boot_info.physmmap_n] = new_entry;
    boot_info.physmmap_n++;
}
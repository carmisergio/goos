#include "boot/multiboot.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "log.h"
#include "boot/multiboot_structs.h"
#include "boot_info.h"
#include "mem/c_paging.h"
#include "mem/vmem.h"
#include "panic.h"

// Internal function prototypes
static void _mb_setup_boot_info(multiboot_info_t *mbd);
static void _mb_setup_boot_info_physmmap(multiboot_memory_map_t *mmap, uint32_t n);
static void _mb_add_physmmap_entry(uint32_t start, uint32_t size);

// TODO: Remove
static void _display_phys_mmap();

/* Public functions */

void mb_read_data(multiboot_info_t *mbd_phys)
{
    void *mmap_addr;
    uint32_t mmap_length;
    multiboot_info_t *mbd;
    multiboot_memory_map_t *mmap;

    klog("Reading multiboot data...\n");
    klog("Multiboot struct physical address: %x\n", mbd_phys);

    // Map multiboot struct in kernel virtual memory
    mbd = vmem_map_range_anyk_noalloc(
        mbd_phys, sizeof(multiboot_info_t));

    klog("Multiboot struct virtual address: %x\n", mbd);

    // NOTE: Here we can read the multiboot info struct as it is now
    //       mapped into the KVAS

    // Check if memory map is present in multiboot info struct
    if (!(mbd->flags & MULTIBOOT_INFO_MEM_MAP))
        panic("MB_READ_DATA_NO_MEMMAP");

    vmem_log_vaddrspc();

    // Read memory map information
    mmap_addr = (void *)mbd->mmap_addr;
    mmap_length = mbd->mmap_length;

    klog("Memory map address: %x, length: %d\n", mmap_addr, mmap_length);

    // Unmap multiboot info struct
    vmem_unmap_range_nofree(mbd, sizeof(multiboot_info_t));

    vmem_log_vaddrspc();

    // Map memory map
    mmap = vmem_map_range_anyk_noalloc(
        mmap_addr, sizeof(multiboot_memory_map_t) * mmap_length);

    vmem_log_vaddrspc();
    _mb_setup_boot_info_physmmap(mmap, mmap_length);
    _display_phys_mmap();
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
    // boot_info_phys->physmmap_n = 0;

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
    // boot_info_phys->physmmap[boot_info.physmmap_n] = new_entry;
    // boot_info_phys->physmmap_n++;
}

/*
 * Display available physical memory map
 */
static void _display_phys_mmap()
{
    klog("Available physical memory:\n");
    for (size_t i = 0; i < boot_info.physmmap_n; i++)
    {
        klog(" - Address: %x, size: %d pages\n",
             boot_info.physmmap[i].addr, boot_info.physmmap[i].npages);
    }
}
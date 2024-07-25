#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "log.h"
#include "mem/physmem.h"
#include "boot_info.h"

// TODO: rewrite everything
// Intermediate memory map with Available, REserved by hardware and reserved by software areas
// Get maximum address
// Compute available memory blocks
// Find block that can fit bitmap
// Add bitmap to intermediate memory map as reserved by software
// Initialize bitmap as all zeroes (all reserved)
// For each page in the bitmap, compute its state based on the intermediate memory map
// Enjoy!

// Bitmap global objects
char *_bitmap;
uint32_t _bitmap_pages;

// static void _display_mem_map(multiboot_info_t *mbd);
// static void _construct_phys_mmap_multiboot(multiboot_info_t *mbd);
// static void _add_phys_mmap_entry(uint32_t start, uint32_t size);
static void _display_phys_mmap();
// static uint32_t _calc_max_phys_addr();
// static void _construct_bitmap();
// static void _allocate_bitmap(uint32_t bitmap_size);

/* Public functions */

void physmem_init()
{
    klog("Initializing frame allocator...\n");

    _display_phys_mmap();

    // // Construct bitmap
    // _construct_bitmap();
}

/* Internal functions */

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

// /*
//  * Compute maximum address valid in physical memory
//  */
// static uint32_t _calc_max_phys_addr()
// {
//     uint32_t max = 0;

//     for (size_t i = 0; i < _phys_mmap_n; i++)
//     {
//         // Compute last address of this entry in memory map
//         uint32_t addr = _phys_mmap[i].addr + _phys_mmap[i].npages * PAGE_SIZE;

//         // Check if calculated address is more than previous segments
//         max = addr > max ? addr : max;
//     }

//     return max;
// }

// /*
//  * Construct bitmap
//  */
// static void _construct_bitmap()
// {
//     uint32_t addr_max;

//     // Compute maximum physical address
//     addr_max = _calc_max_phys_addr();
//     klog("Maximum physical address: %x\n", addr_max);

//     // Compute number of pages haandled by the bitmap
//     _bitmap_pages = addr_max / PAGE_SIZE;

//     // Compute size of bitmap in bytes
//     uint32_t bitmap_size = _bitmap_pages / 8; // 8 bits in a byte
//     klog("Bitmap size: %d bytes\n", bitmap_size);

//     // Find place to put bitmap
//     _allocate_bitmap(bitmap_size);
//     klog("Bitmap addr: %x", _bitmap);

//     // Fill bitmap with 0 (all not free)
//     memset(_bitmap, 0x00, bitmap_size);

//     //
// }

// static void _allocate_bitmap(uint32_t bitmap_size)
// {

//     // Find place in memory map to put bitmap
//     for (uint32_t i = 0; i < _phys_mmap_n; i++)
//     {
//         uint32_t start, end;
//         uint32_t phys_start = _phys_mmap[i].addr;
//         uint32_t phys_end = phys_start + _phys_mmap[i].npages * PAGE_SIZE;

//         // Handle kernel being in the way
//         if (phys_start > (uint32_t)&_kernel_end || phys_end < (uint32_t)&_kernel_start)
//         {
//             start = phys_start;
//             end = phys_end;
//         }
//         else if (phys_start < (uint32_t)&_kernel_end && phys_end > (uint32_t)&_kernel_end)
//         {
//             start = (uint32_t)&_kernel_end;
//             end = phys_end;
//         }
//         else
//         {
//             // TODO: Handle other cases
//             continue;
//         }

//         // Page-align start address
//         start = ((start + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;

//         // Check if segment is big enough
//         if ((end - start) >= bitmap_size)
//         {
//             _bitmap = (char *)start;
//             return;
//         }
//     }

//     klog("Unable to allocate bitmap!\n");
//     // TODO: panic
// }
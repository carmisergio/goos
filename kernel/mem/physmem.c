#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include "log.h"
#include "mem/physmem.h"
#include "mem/c_paging.h"
#include "boot_info.h"
#include "boot/boot.h"
#include "panic.h"

// Bitmap global objects
char *_bitmap;
uint32_t _bitmap_pages;
uint32_t _free_pages;

#define MAX_SRMMAP_ENTRIES 4

/* Entry in the software reserved memory map used during
   physical memory initialization */
struct srmmap_entry
{
    uint32_t addr;
    uint32_t npages;
};
typedef struct srmmap_entry srmmap_entry_t;

/* Add entry to the software reserved memory map
 * This macro requries srmmap and srmmap_n to be in scope
 * Length parameter is in bytes, conversion to pages is handled here
 */
#define MAKE_SRMMAP_ENTRY(a, l)                                                        \
    {                                                                                  \
        srmmap[srmmap_n].addr = (uint32_t)(a);                                         \
        srmmap[srmmap_n].npages = ((uint32_t)(l) + MEM_PAGE_SIZE - 1) / MEM_PAGE_SIZE; \
        srmmap_n++;                                                                    \
    }

/*
 * Page align size
 */
#define PAGE_ALIGN_SIZE(s) ((s + MEM_PAGE_SIZE - 1) / MEM_PAGE_SIZE) * MEM_PAGE_SIZE

// Private function prototypes
static void _display_phys_mmap();
static void _display_srmmap(srmmap_entry_t *srmmap, uint32_t srmmap_n);
static uint32_t _calc_addr_space_size();
static uint32_t _allocate_bitmap(uint32_t size, srmmap_entry_t *srmmap, uint32_t srmmap_n);
static bool _is_physmem(uint32_t addr);
static bool _is_softres(srmmap_entry_t *srmmap, uint32_t srmmap_n, uint32_t addr);
static void _initialize_bitmap(srmmap_entry_t *srmmap, uint32_t srmmap_n);
static inline bool _is_page_free(uint32_t page);
static inline void _mark_page_free(uint32_t page);
static inline void _mark_page_used(uint32_t page);

/* Public functions */

void physmem_init()
{
    // Make space for software reserved memory map
    srmmap_entry_t srmmap[MAX_SRMMAP_ENTRIES];
    uint32_t srmmap_n = 0;

    uint32_t max_addr, bitmap_addr;

    klog("Initializing frame allocator...\n");
    _display_phys_mmap();

    // Mark the kernel load area as software reserved
    MAKE_SRMMAP_ENTRY((uint32_t)&_kernel_start,
                      (uint32_t)&_kernel_end - (uint32_t)&_kernel_start);

    // Compute maximum physical address
    max_addr = _calc_addr_space_size();
    klog("Maximum physical address: %x\n", max_addr);

    // Compute number of pages haandled by the bitmap
    _bitmap_pages = max_addr / MEM_PAGE_SIZE;

    // Compute size of bitmap in bytes
    uint32_t bitmap_size = PAGE_ALIGN_SIZE(_bitmap_pages / 8); // 8 bits in a byte
    klog("Bitmap size: %d bytes\n", bitmap_size);

    // Find place to put bitmap
    bitmap_addr = _allocate_bitmap(bitmap_size, srmmap, srmmap_n);
    _bitmap = (char *)bitmap_addr;
    klog("Bitmap addr: %x\n", bitmap_addr);

    // Mark bitmap memory as software reserved
    MAKE_SRMMAP_ENTRY(bitmap_addr, bitmap_size);

    _display_srmmap(srmmap, srmmap_n);

    // Initialize bitmap from the physical memory and software reserved maps
    memset(_bitmap, 0x00, _bitmap_pages / 8); // Set bitmap to all zeroes (all reserved)
    _initialize_bitmap(srmmap, srmmap_n);

    klog("Free memory: %d KiB\n",
         _free_pages * MEM_PAGE_SIZE / 1024); // 1024 bytes in a KiB
}

/*
 * Allocate one page of physical memory
 * Returns PHYSMEM_NULL (0xFFFFFFFF) on failure
 */
void *physmem_alloc()
{
    uint32_t cur_page;
    for (uint32_t i = _bitmap_pages; i != 0; i--)
    {
        cur_page = i - 1; // Conversion used for iteration

        // Check if page is free
        if (_is_page_free(cur_page))
        {
            // Mark page as allocated
            _mark_page_used(cur_page);

            return (void *)(cur_page * MEM_PAGE_SIZE);
        }
    }

    return (void *)PHYSMEM_NULL;
}

/*
 * Free page of physical memory
 * Panics on double free or free nonexistent page
 */
void physmem_free(void *addr)
{
    uint32_t page = (uint32_t)addr / MEM_PAGE_SIZE;

    // Check state of page in bitmap
    if (physmem_is_free(addr))
        panic("PHYSMEM_DOUBLE_FREE");

    // Free page
    _mark_page_free(page);
}

/**
 * Checks if a page of physical memory is free
 * Panics on nonexistent page
 */
bool physmem_is_free(void *addr)
{
    uint32_t page = (uint32_t)addr / MEM_PAGE_SIZE;

    return _is_page_free(page);
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

/*
 * Display software reserved memory map
 */
static void _display_srmmap(srmmap_entry_t *srmmap, uint32_t srmmap_n)
{
    klog("Software reserved memory:\n");
    for (uint32_t i = 0; i < srmmap_n; i++)
    {
        klog(" - Address: %x, size: %d (%x) pages\n",
             srmmap[i].addr, srmmap[i].npages, srmmap[i].npages);
    }
}

/*
 * Compute length of address space until last area of physical memory
 */
static uint32_t _calc_addr_space_size()
{
    uint32_t max = 0;

    for (size_t i = 0; i < boot_info.physmmap_n; i++)
    {
        // Compute last address of this entry in memory map
        uint32_t addr = boot_info.physmmap[i].addr + boot_info.physmmap[i].npages * MEM_PAGE_SIZE;

        // Check if calculated address is more than previous segments
        max = addr > max ? addr : max;
    }

    return max;
}

/*
 * Find space in physical memory to put the memory map
 * Panics on failure
 */
static uint32_t _allocate_bitmap(uint32_t size, srmmap_entry_t *srmmap,
                                 uint32_t srmmap_n)
{
    uint32_t cur_page;
    uint32_t cur_run = 0; // Current run of free pages
    uint32_t npages = size / MEM_PAGE_SIZE;
    // uint32_t begin_addr;  // Beginning address of current run

    /* Iterate over all pages in the address space, check if its in physical
       memory, check if it's NOT software reserved, and find run of enough
       free pages */
    for (uint32_t i = _bitmap_pages; i != 0; i--)
    {
        cur_page = i - 1; // Conversion used for iteration

        if (_is_physmem(cur_page * MEM_PAGE_SIZE) &&
            !_is_softres(srmmap, srmmap_n, cur_page * MEM_PAGE_SIZE))
        {
            // Increment number of bytes in run
            cur_run++;

            // Check if run is sufficient to fit the bitmap
            if (cur_run >= npages)
                return cur_page * MEM_PAGE_SIZE;
        }
    }

    // Check if run is sufficient to fit the bitmap
    if (cur_run >= npages)
        return 0;

    panic("PHYSMEM_NO_MEM_FOR_BITMAP");
    return 0;
}

/*
 * Returns true if the address is in physical memory,
 * according to the physical memory map
 */
static bool _is_physmem(uint32_t addr)
{
    for (uint32_t i = 0; i < boot_info.physmmap_n; i++)
    {
        if (addr >= boot_info.physmmap[i].addr &&
            addr < boot_info.physmmap[i].addr +
                       boot_info.physmmap[i].npages * MEM_PAGE_SIZE)
            return true;
    }

    return false;
}

/*
 * Returns true if the address is software reserved
 * according to the software reserved memory map
 */
static bool _is_softres(srmmap_entry_t *srmmap, uint32_t srmmap_n, uint32_t addr)
{
    for (uint32_t i = 0; i < srmmap_n; i++)
    {
        if (addr >= srmmap[i].addr &&
            addr < srmmap[i].addr + srmmap[i].npages * MEM_PAGE_SIZE)
            return true;
    }

    return false;
}

/*
 * Construct bitmap
 */
static void _initialize_bitmap(srmmap_entry_t *srmmap, uint32_t srmmap_n)
{
    _free_pages = 0;

    // Set free pages as free
    for (uint32_t page = 0; page < _bitmap_pages; page++)
    {
        // Both physmmap and srmmap store page aligned areas,
        // so we only need to check the starting address
        if (_is_physmem(page * MEM_PAGE_SIZE) &&
            !_is_softres(srmmap, srmmap_n, page * MEM_PAGE_SIZE))
        {
            _mark_page_free(page);
        }
    }
}

static inline bool _is_page_free(uint32_t page)
{
    // Check if page exists
    if (page >= _bitmap_pages)
    {
        panic("PHYSMEM_INVALID_PAGE_INT");
    }
    return (_bitmap[page / 8] & (1 << (page % 8))) != 0;
}

static inline void _mark_page_free(uint32_t page)
{
    // Check state of page in bitmap
    if (_is_page_free(page))
        panic("PHYSMEM_DOUBLE_FREE_INT");

    // Free page
    _bitmap[page / 8] |= (1 << (page % 8));
    _free_pages++; // Count free pages
}

static inline void _mark_page_used(uint32_t page)
{
    // Check state of page in bitmap
    if (!_is_page_free(page))
        panic("PHYSMEM_DOUBLE_ALLOC_INT");

    // Mark page as used
    _bitmap[page / 8] &= ~(1 << (page % 8));
    _free_pages++; // Count free pages
}
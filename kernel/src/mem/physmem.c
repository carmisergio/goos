#include "mem/physmem.h"

#include <stddef.h>
#include <string.h>

#include "log.h"
#include "mem/vmem.h"
#include "mem/const.h"
#include "boot/boot_info.h"
#include "boot/boot.h"
#include "panic.h"

// Bitmap global objects
char *physmem_bitmap;
uint32_t physmem_bitmap_pages;
uint32_t physmem_first_free_page;

// Accounting information
uint32_t physmem_free_pages;

#define MAX_SRMMAP_ENTRIES 4
#define ISADMA_MEM_LIMIT (16 * 1024 * 1024) // 16M
#define ISADMA_BOUNDARY_SIZE (64 * 1024)    // 64K

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
static void debug_phys_mmap();
static uint32_t calc_addr_space_size();
static uint32_t allocate_bitmap(uint32_t size, srmmap_entry_t *srmmap, uint32_t srmmap_n);
static bool is_physmem(uint32_t addr);
static bool is_softres(srmmap_entry_t *srmmap, uint32_t srmmap_n, uint32_t addr);
static void initialize_bitmap(srmmap_entry_t *srmmap, uint32_t srmmap_n);
static inline bool is_page_free(uint32_t page);
static inline void mark_page_free(uint32_t page);
static inline void mark_page_free_nockeck(uint32_t page);
static inline void mark_page_used(uint32_t page);

/* Public functions */

void physmem_init()
{
    // Make space for software reserved memory map
    srmmap_entry_t srmmap[MAX_SRMMAP_ENTRIES];
    uint32_t srmmap_n = 0;

    uint32_t max_addr, bitmap_paddr;

    kprintf("[PHYSMEM] Initializing...\n");
    debug_phys_mmap();

    // Mark the kernel load area as software reserved
    MAKE_SRMMAP_ENTRY(KERNEL_PHYS_ADDR,
                      (uint32_t)&_kernel_end - (uint32_t)&_kernel_start);

    // Compute maximum physical address
    max_addr = calc_addr_space_size();
    kprintf("Maximum physical address: %x\n", max_addr);

    // Compute number of pages haandled by the bitmap
    physmem_bitmap_pages = max_addr / MEM_PAGE_SIZE;

    // Compute size of bitmap in bytes
    uint32_t bitmap_size = PAGE_ALIGN_SIZE(physmem_bitmap_pages / 8); // 8 bits in a byte
    kprintf("Bitmap size: %d bytes\n", bitmap_size);

    // Find place to put bitmap
    bitmap_paddr = allocate_bitmap(bitmap_size, srmmap, srmmap_n);
    kprintf("Bitmap physical addr: %x\n", bitmap_paddr);

    // Mark bitmap memory as software reserved
    MAKE_SRMMAP_ENTRY(bitmap_paddr, bitmap_size);

    // Map bitmap into kVAS
    physmem_bitmap = vmem_map_range_anyk_noalloc((void *)bitmap_paddr, bitmap_size);
    if (physmem_bitmap == NULL)
        panic("PHYSMEM_INIT_BITMAP_MAP_FAIL", "Unable to map bitmap into VAS");

    kprintf("Bitmap virtual addr: %x\n", physmem_bitmap);

    // Initialize bitmap from the physical memory and software reserved maps
    memset(physmem_bitmap, 0x00, physmem_bitmap_pages / 8); // Set bitmap to all zeroes (all reserved)
    initialize_bitmap(srmmap, srmmap_n);

    kprintf("Free memory: %d KiB\n",
            physmem_free_pages * MEM_PAGE_SIZE / 1024); // 1024 bytes in a KiB
}

void *physmem_alloc()
{
    uint32_t cur_page;
    for (uint32_t i = physmem_first_free_page + 1; i != 0; i--)
    {
        cur_page = i - 1; // Conversion used for iteration

        // Check if page is free
        if (is_page_free(cur_page))
        {
            // Mark page as allocated
            mark_page_used(cur_page);

            return (void *)(cur_page * MEM_PAGE_SIZE);
        }
    }

    return (void *)PHYSMEM_NULL;
}

void *physmem_alloc_n(uint32_t n)
{
    uint32_t cur_page, count = 0;
    for (uint32_t i = physmem_first_free_page + 1; i != 0; i--)
    {
        cur_page = i - 1; // Conversion used for iteration

        // Check if page is free
        if (is_page_free(cur_page))
        {
            // Count free page
            count++;

            // If enough free pages have been found
            if (count >= n)
            {
                // Mark all pages as allocated
                for (uint32_t page = cur_page; page < cur_page + n; page++)
                    mark_page_used(page);

                // Return address of first page
                return (void *)(cur_page * MEM_PAGE_SIZE);
            }
        }
        else
        {
            // Reset count
            count = 0;
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

    // Free page
    mark_page_free(page);
}

void physmem_free_n(void *addr, uint32_t n)
{
    for (size_t i = 0; i < n; i++)
        physmem_free((char *)addr + n * MEM_PAGE_SIZE);
}

/**
 * Checks if a page of physical memory is free
 * Panics on nonexistent page
 */
bool physmem_is_free(void *addr)
{
    uint32_t page = (uint32_t)addr / MEM_PAGE_SIZE;

    return is_page_free(page);
}

void *physmem_alloc_isadma(uint32_t n)
{
    uint32_t run = 0;
    for (uint32_t i = 0; i < ISADMA_MEM_LIMIT / MEM_PAGE_SIZE; i++)
    {
        // If page is first of new 64K block, reset run
        if ((i * MEM_PAGE_SIZE) % ISADMA_BOUNDARY_SIZE == 0)
            run = 0;

        // Check if page is free
        if (is_page_free(i))
        {
            // Count free page
            run++;

            // If enough free pages have been found
            if (run >= n)
            {
                // Mark all pages as allocated
                for (uint32_t page = i - n + 1; page <= i; page++)
                    mark_page_used(page);

                // Return address of first page
                return (void *)((i - n + 1) * MEM_PAGE_SIZE);
            }
        }
        else
        {
            // Reset run
            run = 0;
        }
    }

    return (void *)PHYSMEM_NULL;
}

/* Internal functions */

/*
 * Display available physical memory map
 */
static void debug_phys_mmap()
{
    kprintf("Available physical memory:\n");
    for (size_t i = 0; i < boot_info.physmmap_n; i++)
    {
        kprintf(" - Address: %x, size: %d pages\n",
                boot_info.physmmap[i].addr, boot_info.physmmap[i].npages);
    }
}

/*
 * Compute length of address space until last area of physical memory
 */
static uint32_t calc_addr_space_size()
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
static uint32_t allocate_bitmap(uint32_t size, srmmap_entry_t *srmmap,
                                uint32_t srmmap_n)
{
    uint32_t cur_page;
    uint32_t cur_run = 0; // Current run of free pages
    uint32_t npages = size / MEM_PAGE_SIZE;
    // uint32_t begin_addr;  // Beginning address of current run

    /* Iterate over all pages in the address space, check if its in physical
       memory, check if it's NOT software reserved, and find run of enough
       free pages */
    for (uint32_t i = physmem_bitmap_pages; i != 0; i--)
    {
        cur_page = i - 1; // Conversion used for iteration

        if (is_physmem(cur_page * MEM_PAGE_SIZE) &&
            !is_softres(srmmap, srmmap_n, cur_page * MEM_PAGE_SIZE))
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

    panic("PHYSMEM_NO_MEM_FOR_BITMAP", "Couldn't find space for physical memory bitmap");
    return 0;
}

/*
 * Returns true if the address is in physical memory,
 * according to the physical memory map
 */
static bool is_physmem(uint32_t addr)
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
static bool is_softres(srmmap_entry_t *srmmap, uint32_t srmmap_n, uint32_t addr)
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
static void initialize_bitmap(srmmap_entry_t *srmmap, uint32_t srmmap_n)
{
    physmem_free_pages = 0;

    // Set free pages as free
    for (uint32_t page = 0; page < physmem_bitmap_pages; page++)
    {
        // Both physmmap and srmmap store page aligned areas,
        // so we only need to check the starting address
        if (is_physmem(page * MEM_PAGE_SIZE) &&
            !is_softres(srmmap, srmmap_n, page * MEM_PAGE_SIZE))
        {
            mark_page_free_nockeck(page);
        }
    }
}

static inline bool is_page_free(uint32_t page)
{
    // Check if page exists
    if (page >= physmem_bitmap_pages)
    {
        panic("PHYSMEM_INVALID_PAGE_INT", "Page does not exist");
    }
    return (physmem_bitmap[page / 8] & (1 << (page % 8))) != 0;
}

static inline void mark_page_free(uint32_t page)
{
    // Check state of page in bitmap
    if (is_page_free(page))
        panic("PHYSMEM_DOUBLE_FREE_INT", "Can't free a page that is already free");

    // Free page
    physmem_bitmap[page / 8] |= (1 << (page % 8));
    physmem_free_pages++; // Count free pages

    // Set first free page
    if (page > physmem_first_free_page)
        physmem_first_free_page = page;
}

static inline void mark_page_free_nockeck(uint32_t page)
{
    // Free page
    physmem_bitmap[page / 8] |= (1 << (page % 8));
    physmem_free_pages++; // Count free pages

    // Set first free page
    if (page > physmem_first_free_page)
        physmem_first_free_page = page;
}

static inline void mark_page_used(uint32_t page)
{
    // Check state of page in bitmap
    if (!is_page_free(page))
        panic("PHYSMEM_DOUBLE_ALLOC_INT", "Trying to mark page as allocated, but it is already allocated");

    // Mark page as used
    physmem_bitmap[page / 8] &= ~(1 << (page % 8));
    physmem_free_pages++; // Count free pages

    // Set first free page
    if (page == physmem_first_free_page)
        physmem_first_free_page = page - 1;
}
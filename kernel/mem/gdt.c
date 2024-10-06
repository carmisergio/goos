#include <stdint.h>

#include "string.h"

#include "mem/gdt.h"

#include "mem/const.h"
#include "mem/mem.h"
#include "panic.h"

// GDT access byte flags
#define GDT_ACCESSED 0x1
#define GDT_RW (0x1 << 1)
#define GDT_DC (0x1 << 2)
#define GDT_E (0x1 << 3)
#define GDT_S (0x1 << 4)
#define GDT_USER (0x3 << 5)
#define GDT_KERNEL (0x0 << 5)
#define GDT_P (0x1 << 7)
#define GDT_TSS32_AVAILABLE 0x9

// GDT flags
#define GDT_L (0x1 << 1)
#define GDT_DB (0x1 << 2)
#define GDT_G (0x1 << 3)

// Representation of an entry in the GDT
struct __attribute__((packed)) gdt_entry
{
    uint32_t limit_low : 16;
    uint32_t base_low : 24;
    uint32_t access : 8; // Access byte
    uint32_t limit_high : 4;
    uint32_t flags : 4;
    uint32_t base_high : 8;
};
typedef struct gdt_entry gdt_entry_t;

// Representatino of a Task State Segment
typedef volatile struct tss_struct
{
    uint16_t link;
    uint16_t _link_h;

    uint32_t esp0;
    uint16_t ss0;
    uint16_t _ss0_h;

    uint32_t esp1;
    uint16_t ss1;
    uint16_t _ss1_h;

    uint32_t esp2;
    uint16_t ss2;
    uint16_t _ss2_h;

    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;

    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;

    uint32_t esp;
    uint32_t ebp;

    uint32_t esi;
    uint32_t edi;

    uint16_t es;
    uint16_t _es_h;

    uint16_t cs;
    uint16_t _cs_h;

    uint16_t ss;
    uint16_t _ss_h;

    uint16_t ds;
    uint16_t _ds_h;

    uint16_t fs;
    uint16_t _fs_h;

    uint16_t gs;
    uint16_t _gs_h;

    uint16_t ldt;
    uint16_t _ldt_h;

    uint16_t trap;
    uint16_t iomap;

    uint32_t ssp;

} tss_struct_t;

// Descriptor needed to load the GDT
struct __attribute__((packed)) gdt_pointer
{
    uint16_t limit;
    uint32_t base;
};
typedef struct gdt_pointer gdt_pointer_t;

// Load gdt (assembly function)
void load_gdt(void *gdt_desc);

// Function prototypes
void set_up_tss();

// Task State segment global object
// We use only one TSS for all tasks
static tss_struct_t tss;

// GDT global object
gdt_entry_t gdt[6] = {
    {.flags = 0}, // Null segment
    // Kernel code segment
    {
        // Span
        .base_low = 0,
        .base_high = 0,
        .limit_low = 0xFFFF,
        .limit_high = 0xF,
        // Access byte
        .access = GDT_P | GDT_KERNEL | GDT_S | GDT_E | GDT_RW,
        // Flags
        .flags = GDT_G | GDT_DB,
    },
    // Kernel data segment
    {
        // Span
        .base_low = 0,
        .base_high = 0,
        .limit_low = 0xFFFF,
        .limit_high = 0xF,
        // Access byte
        .access = GDT_P | GDT_KERNEL | GDT_S | GDT_RW,
        // Flags
        .flags = GDT_G | GDT_DB,
    },
    // User code segment
    {
        // Span
        .base_low = 0,
        .base_high = 0,
        .limit_low = 0xFFFF,
        .limit_high = 0xF,
        // Access byte
        .access = GDT_P | GDT_USER | GDT_S | GDT_E | GDT_RW,
        // Flags
        .flags = GDT_G | GDT_DB,
    },
    // User data segment
    {
        // Span
        .base_low = 0,
        .base_high = 0,
        .limit_low = 0xFFFF,
        .limit_high = 0xF,
        // Access byte
        .access = GDT_P | GDT_USER | GDT_S | GDT_RW,
        // Flags
        .flags = GDT_G | GDT_DB,
    },
    // TSS Segment
    {
        // Base and limit are set at runtime
        // Access byte
        .access = GDT_P | GDT_KERNEL | GDT_TSS32_AVAILABLE,
        // Flags
        .flags = 0,
    },
};

void setup_gdt()
{
    // Load GDT
    gdt_pointer_t gdt_pointer = {.limit = sizeof(gdt), .base = (uint32_t)&gdt};
    load_gdt(&gdt_pointer);

    set_up_tss();
}

/**
 * Sets up TSS with correct interrupt stack and
 * loads the task register
 */
void set_up_tss()
{
    // Allocate interrupt stack
    memset((void *)&tss, 0x0, sizeof(tss_struct_t));
    tss.ss0 = GDT_SEGMENT_KDATA;
    tss.esp0 = (uint32_t)mem_palloc_k(INTERRUPT_STACK_PAGES);
    tss.iomap = sizeof(tss_struct_t);

    // Compute limit
    uint32_t limit = (uint32_t)&tss + sizeof(tss_struct_t);

    // Set TSS address in GDT
    uint32_t gdt_tss_index = GDT_SEGMENT_TSS / sizeof(gdt_entry_t);
    gdt[gdt_tss_index].base_low = ((uint32_t)&tss & 0xFFFFFF);
    gdt[gdt_tss_index].base_high = ((uint32_t)&tss) >> 24;
    gdt[gdt_tss_index].limit_low = limit & 0xFFFF;
    gdt[gdt_tss_index].limit_high = limit >> 16;
    gdt[gdt_tss_index].flags |= GDT_P; // Set present flag

    // Check allocation result
    if ((void *)tss.esp0 == MEM_FAIL)
        panic("SET_UP_TSS_INT_STACK_ALLOC_FAIL");

    // Load TSS
    asm("ltr %0" : : "r"(GDT_SEGMENT_TSS));
}
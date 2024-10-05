#include <stdint.h>

#include "mem/gdt.h"

// Representation of an entry in the GDT
struct __attribute__((packed)) gdt_entry
{
    uint32_t limit_low : 16;
    uint32_t base_low : 24;
    uint32_t a : 1;   // Accessed flag
    uint32_t rw : 1;  // Read / Write bit (different in code and data segments)
    uint32_t dc : 1;  // Direction/Conforming bit
    uint32_t e : 1;   // Executable bit
    uint32_t s : 1;   // Descriptor type bit
    uint32_t dpl : 2; // Descriptor privilege level
    uint32_t p : 1;   // Present bit
    uint32_t limit_high : 4;
    uint32_t rfu : 1; // Available for use
    uint32_t l : 1;   // Long mode bit
    uint32_t db : 1;  // Size bit
    uint32_t g : 1;   // Granularity (page if set)
    uint32_t base_high : 8;
};
typedef struct gdt_entry gdt_entry_t;

// Descriptor needed to load the GDT
struct __attribute__((packed)) gdt_pointer
{
    uint16_t limit;
    uint32_t base;
};
typedef struct gdt_pointer gdt_pointer_t;

// Load gdt (assembly function)
void load_gdt(void *gdt_desc);

// GDT global object
gdt_entry_t gdt[5] = {
    {.p = 0}, // Null segment
    // Kernel code segment
    {
        // Span
        .base_low = 0,
        .base_high = 0,
        .limit_low = 0xFFFF,
        .limit_high = 0xF,
        // Access byte
        .rw = 1,
        .dc = 0,
        .e = 1,
        .s = 1,
        .dpl = 0,
        .p = 1,
        // Flags
        .g = 1,
        .db = 1,
    },
    // Kernel data segment
    {
        // Span
        .base_low = 0,
        .base_high = 0,
        .limit_low = 0xFFFF,
        .limit_high = 0xF,
        // Access byte
        .rw = 1,
        .dc = 0,
        .e = 0,
        .s = 1,
        .dpl = 0,
        .p = 1,
        // Flags
        .g = 1,
        .db = 1,
    },
    // User code segment
    {
        // Span
        .base_low = 0,
        .base_high = 0,
        .limit_low = 0xFFFF,
        .limit_high = 0xF,
        // Access byte
        .rw = 1,
        .dc = 0,
        .e = 1,
        .s = 1,
        .dpl = 3,
        .p = 1,
        // Flags
        .g = 1,
        .db = 1,
    },
    // User data segment
    {
        // Span
        .base_low = 0,
        .base_high = 0,
        .limit_low = 0xFFFF,
        .limit_high = 0xF,
        // Access byte
        .rw = 1,
        .dc = 0,
        .e = 0,
        .s = 1,
        .dpl = 3,
        .p = 1,
        // Flags
        .g = 1,
        .db = 1,
    },
};

void setup_gdt()
{
    gdt_pointer_t gdt_pointer = {.limit = sizeof(gdt), .base = (uint32_t)&gdt};
    // Load GDT
    load_gdt(&gdt_pointer);
}
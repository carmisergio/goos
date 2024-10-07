#include "int/idt.h"

#include <stdint.h>

#include "mem/const.h"
#include "log.h"

// IDT Flags
#define IDT_TYPE_TASK 0x5
#define IDT_TYPE_INT16 0x6
#define IDT_TYPE_TRAP16 0x7
#define IDT_TYPE_INT32 0xE
#define IDT_TYPE_TRAP32 0xF
#define IDT_KERNEL (0x0 << 5)
#define IDT_P (0x1 << 7)

// Semgent selector flags
#define SEGSEL_KERNEL 0x0

// Interrupt vectors (defined in vectors.S)
extern void int_vector_0();
extern void int_vector_1();
extern void int_vector_2();
extern void int_vector_3();
extern void int_vector_4();
extern void int_vector_5();
extern void int_vector_6();
extern void int_vector_7();
extern void int_vector_8();
extern void int_vector_9();
extern void int_vector_10();
extern void int_vector_11();
extern void int_vector_12();
extern void int_vector_13();
extern void int_vector_14();
extern void int_vector_15();
extern void int_vector_16();
extern void int_vector_17();
extern void int_vector_18();
extern void int_vector_19();
extern void int_vector_20();
extern void int_vector_21();
extern void int_vector_22();
extern void int_vector_23();
extern void int_vector_24();
extern void int_vector_25();
extern void int_vector_26();
extern void int_vector_27();
extern void int_vector_28();
extern void int_vector_29();
extern void int_vector_30();
extern void int_vector_31();
extern void int_vector_32();

// Load IDT with LIDT instruction
// Assembly function defined in load_idt.S
void load_idt(void *idt_pointer);

// Entry in the IDT
typedef struct __attribute__((packed))
{
    uint16_t offset_low;
    uint16_t segment;
    uint8_t reserved;
    uint8_t flags;
    uint16_t offset_high;
} idt_entry_t;

// Pointer structure used for LIDT
typedef struct __attribute__((packed))
{
    uint16_t limit;
    uint32_t base;
} idt_pointer_t;

// Global IDT object
volatile idt_entry_t idt[IDT_ENTRY_N];

// Shortcut for initializing idt_entry_t instances
static inline void idt_int_set_entry(const uint8_t i, void (*isr)())
{
    // ISR addreses
    idt[i].offset_low = (uint32_t)isr & 0xFFFF;
    idt[i].offset_high = (uint32_t)isr >> 16;

    // Always run ISRs in the kernel code segment
    idt[i].segment = GDT_SEGMENT_KCODE | SEGSEL_KERNEL; /* Construct segment selector */

    // Flags
    idt[i].flags = IDT_TYPE_INT32 | IDT_KERNEL | IDT_P;
}

void set_up_idt()
{
    // Set up gates
    idt_int_set_entry(0, int_vector_0);
    idt_int_set_entry(1, int_vector_1);
    idt_int_set_entry(2, int_vector_2);
    idt_int_set_entry(3, int_vector_3);
    idt_int_set_entry(4, int_vector_4);
    idt_int_set_entry(5, int_vector_5);
    idt_int_set_entry(6, int_vector_6);
    idt_int_set_entry(7, int_vector_7);
    idt_int_set_entry(8, int_vector_8);
    idt_int_set_entry(9, int_vector_9);
    idt_int_set_entry(10, int_vector_10);
    idt_int_set_entry(11, int_vector_11);
    idt_int_set_entry(12, int_vector_12);
    idt_int_set_entry(13, int_vector_13);
    idt_int_set_entry(14, int_vector_14);
    idt_int_set_entry(15, int_vector_15);
    idt_int_set_entry(16, int_vector_16);
    idt_int_set_entry(17, int_vector_17);
    idt_int_set_entry(18, int_vector_18);
    idt_int_set_entry(19, int_vector_19);
    idt_int_set_entry(22, int_vector_22);
    idt_int_set_entry(21, int_vector_21);
    idt_int_set_entry(22, int_vector_22);
    idt_int_set_entry(23, int_vector_23);
    idt_int_set_entry(24, int_vector_24);
    idt_int_set_entry(25, int_vector_25);
    idt_int_set_entry(26, int_vector_26);
    idt_int_set_entry(27, int_vector_27);
    idt_int_set_entry(28, int_vector_28);
    idt_int_set_entry(29, int_vector_29);
    idt_int_set_entry(30, int_vector_30);
    idt_int_set_entry(31, int_vector_31);

    // Construct IDT pointer
    idt_pointer_t idt_pointer = {.base = (uint32_t)&idt, .limit = sizeof(idt)};

    // Load IDT
    load_idt(&idt_pointer);
}
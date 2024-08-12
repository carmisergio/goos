#ifndef _MEM_C_PAGING_H
#define _MEM_C_PAGING_H 1

// Addresses
#define KERNEL_VIRT_ADDR 0xC0100000 // Kernel virtual address when paging is enabled (must be page aligned)
#define KERNEL_PHYS_ADDR 0x100000   // Kernel physical address when loaded by bootloader (must be page aligned)
#define KERNEL_OFFSET (KERNEL_VIRT_ADDR - KERNEL_PHYS_ADDR)
#define PAGE_DIR_SELFREF_ADDR 0xFFC00000 // Address to which the page directory is self referenced
#define KERNEL_VAS_START 0xC0000000      // First address of the keernel half of the address space

// Sizes
#define MEM_PAGE_SIZE 4096                         // Memory page
#define PDE_ADDR_SPACE_SIZE (1024 * MEM_PAGE_SIZE) // Address space mapped by one PDE
#define PDE_NUM 1024                               // Number of page directory entries
#define PTE_NUM 1024                               // Number of page table entries

// PDE flags
#define PDE_FLAG_PRESENT 0x1

// PTE flags
#define PTE_FLAG_PRESENT 0x1

// CR0 bits
#define CR0_PE 0x1
#define CR0_PG 0x80000000

#endif
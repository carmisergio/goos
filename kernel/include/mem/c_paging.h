#ifndef _MEM_C_PAGING_H
#define _MEM_C_PAGING_H 1

// Virtual address where the kernel will be mapped to
#define KERNEL_VIRT_ADDR 0xC0100000

// Physical address at whch the kernel will be loaded by the bootloader
#define KERNEL_PHYS_ADDR 0x100000

#define KERNEL_OFFSET (KERNEL_VIRT_ADDR - KERNEL_PHYS_ADDR)

// Memory page size (4 KiB)
#define MEM_PAGE_SIZE 4096

// Size of the address space covered by one PDE (4 MiB)
#define PDE_ADDR_SPACE_SIZE (1024 * MEM_PAGE_SIZE)

// PDE flags
#define PDE_FLAG_PRESENT 0x1

#endif
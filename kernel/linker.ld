// This file will be processed by the C preprocessor before being
// used for linking
#include "mem/const.h"

ENTRY(_start)

SECTIONS
{
	/* Address of Higher Half kernel */
	. = KERNEL_VIRT_ADDR;
	

    /* Multiboot header and kernel code */
	.text BLOCK(4K) : AT(ADDR(.text) - KERNEL_OFFSET)
	{
	_kernel_start = .;
		*(.multiboot)
		*(.text)
	}

    /* Read only data */
	.rodata BLOCK(4K) : AT(ADDR(.rodata) - KERNEL_OFFSET)
	{
		*(.rodata)
	}

	/* Read-write data (initialized) */
	.data BLOCK(4K) : AT(ADDR(.data) - KERNEL_OFFSET)
	{
		*(.data)
	}

	/* Read-write data (uninitialized) and stack */
	.bss BLOCK(4K) : AT(ADDR(.bss) - KERNEL_OFFSET)
	{
		*(COMMON)
		*(.bss)
	PROVIDE(_kernel_end = .); /* Use PROVIDE() for assertion */
	}

	/* Make sure the kernel can be completely mapped using a single page table */
	ASSERT((_kernel_end <= (KERNEL_VIRT_ADDR + PDE_ADDR_SPACE_SIZE)), "Kernel exceeds single page table")
}


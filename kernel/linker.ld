ENTRY(_start)

SECTIONS
{
/* Load address: 1 MB (Beginning of High Memory) */
. = 1M;
	

    /* Multiboot header and kernel code */
	.text BLOCK(4K) : ALIGN(4K)
	{
_kernel_start = .;
		*(.multiboot)
		*(.text)
	}

    /* Read only data */
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	/* Read-write data (initialized) */
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}

	/* Read-write data (uninitialized) and stack */
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
_kernel_end = .;
	}
	
}


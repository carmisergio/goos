ENTRY(_start)

SECTIONS
{
	. = 1M;
	
    /* Code */
	.text BLOCK(4K):
	{
		*(.text)
	}

    /* Read only data */
	.rodata BLOCK(4K):
	{
		*(.rodata)
	}

	/* Read-write data (initialized) */
	.data BLOCK(4K):
	{
		*(.data)
	}

	/* Read-write data (uninitialized) */
	.bss BLOCK(4K):
	{
		*(COMMON)
		*(.bss)
	}
}


#include "mem/c_paging.h"

# Declare constants for the multiboot header.
.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # multiboot magic number
.set CHECKSUM, -(MAGIC + FLAGS) # checksum
	
# Higher half kernel load address

# Multiboot header
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Kernel stack
.section .bss
.align 16
stack_bottom:
 .skip 16385 # 16 KiB
stack_top:
	
# Bootstrap page tables
 .section .bss
 .align MEM_PAGE_SIZE
 bootstrap_page_dir:
 	.skip 4096
 bootstrap_page_table_1:
	.skip 4096
 bootstrap_page_table_2:
	.skip 4096
	

# Kernel entry point
.section .text
.global _start
.type _start, @function
_start:
	# Set up bootstrap stack (at physical address for now)
	movl 	$stack_top - KERNEL_OFFSET, %esp

	// Initialize paging
	call _paging_init
	
	// Update stack
	movl 	$stack_top, %esp
	
	# Call global constructors
	call 	_init

	# Transfer control to the kernel
	push 	%eax # Pass multiboot info struct to kernel
	push 	%ebx # Pass mulbitoot magic number to kernel
	call 	kmain

	# Hang if kernel_main unexpectedly returns.
	cli
1:	hlt
	jmp 	1b
.size _start, . - _start

	
/*
 * Paging setup
 * Sets up the paging structure necessary for boostrap, making the folloiwng
 * mappings:
 * - 0 - 0x10000 -> 0 - 0x10000 (Low memory, used for hardware stuff)
 * - 0x10000 - 0x40000 -> 0x10000 - 0x40000  (Kernel identity mapping)
 * - 0x10000 - 0x40000 -> 0xC010000 - 0xC040000  (Kernel higher half mapping)
 * NOTE: This code assumes thtat the kernel's size is less than 3 MB
*/
_paging_init:
	push 	%eax
	push 	%ebx
	push 	%ecx
	push 	%edx
	
	// eax <- Kernel page directory physical address
	mov 	$bootstrap_page_dir - KERNEL_OFFSET, %eax
	
	// Set bootstrap_page_table_1 at starting address 0 in page directory
	mov		$bootstrap_page_table_1 - KERNEL_OFFSET, %ebx
	or		$0x1, %ebx // Set present flag
	mov 	%ebx, (bootstrap_page_dir - KERNEL_OFFSET)

	// Set bootstrap_page_table_2 to map address specified by kernel virtual mapping
	call 	_compute_kern_virt_pde_offset 	// Calculate offset in PDE
	mov		$bootstrap_page_table_2 - KERNEL_OFFSET, %ebx
	or		$0x1, %ebx // Set present flag
	mov 	%ebx, (bootstrap_page_dir - KERNEL_OFFSET)(%edx) // PDE + offset

	push 	%edx
	push 	%ecx
	push 	%ebx
	push 	%eax
	ret
	
/*
 * Compute offset of the higher half kernel mapping page table
 * in the page directory
 * Inputs: 
 * 	 global definition KERNEL_VIRT_ADDR
 * Outputs:
 *   %edx: offset
 */
_compute_kern_virt_pde_offset:
	mov	    $KERNEL_VIRT_ADDR, %edx
	shr		$22, %edx	// Calc offset into page direcotry
						// NOTE: page directory handles the first 12 bits
						// Of the addresses
	shl 	$2, %edx	// Multiply by 4 to obtain byte offset
	ret
#include "mem/const.h"

# Declare constants for the multiboot header.
.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # multiboot magic number
.set CHECKSUM, -(MAGIC + FLAGS) # checksum
.set BOOTLOADER_MAGIC, 0x2BADB002 # Bootloader magic number

# Multiboot header
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Kernel stack
.section .bss
.align 16
.global kernel_stack_top
.type kernel_stack_top, @object
kernel_stack_bottom:
 	.skip 16385 # 16 KiB
kernel_stack_top:
	
# Bootstrap page tables
.section .bss
.align MEM_PAGE_SIZE
.global bootstrap_page_dir
.type bootstrap_page_dir, @object
bootstrap_page_dir:
	.skip 4096
bootstrap_page_table_low:
	.skip 4096
bootstrap_page_table_high:
	.skip 4096
	

# Kernel entry point
.section .text
.global _start
.type _start, @function
_start:
	# Disable interrupts
	cli

	# Set up bootstrap stack (at physical address for now)
	movl 	$kernel_stack_top - KERNEL_OFFSET, %esp
	
	// Check multiboot bootloader magic number
	cmp     $BOOTLOADER_MAGIC, %eax
	jne 	init_error

	// Initialize paging
	call 	_paging_init
	
	// Update stack
	movl 	$kernel_stack_top, %esp
	
	# Call global constructors
	call 	_init
	
	# Transfer control to the kernel
	push 	%ebx # Pass multiboot info struct pointer to kernel
	call 	kmain

	# Hang if kernel_main unexpectedly returns.
init_error:
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
 * NOTE: register eax is not preserved
*/
_paging_init:
	push 	%ebx
	push 	%ecx
	push 	%edx

	mov		$bootstrap_page_dir - KERNEL_OFFSET, %eax       // Pointer to page directory
	
	/********************************************
	*          Set up page directory            *
	********************************************/
	
	// Set up low PDE
	mov 	$bootstrap_page_table_low - KERNEL_OFFSET, %ebx // Pointer to page table
	mov		$0, %edx										// Map address
	call 	_set_page_dir_entry

	// Set up high PDE
	mov 	$bootstrap_page_table_high - KERNEL_OFFSET, %ebx // Pointer to page table
	mov		$KERNEL_VIRT_ADDR, %edx						     // Map address
	call 	_set_page_dir_entry
	
	// Recursively map last PDE entry to the page table itself
	// By doing this, editing the page tables becomes easy
	mov		$bootstrap_page_dir - KERNEL_OFFSET, %ebx       // Pointer to the page directory
	mov		$PAGE_DIR_SELFREF_ADDR, %edx
	call    _set_page_dir_entry

	/********************************************
	*            Set up page tables             *
	********************************************/
	
	// Identity map 000000-100000 (Low memory for initial hardware access)
	mov		$0x000000, %ebx	    // Physical address
	mov		$0x100000, %ecx	    // Size
	mov		$0x000000, %edx	    // Virtual address
	call    _map_address_range 

	// Kernel identity mapping
	mov 	$_kernel_end, %ecx   	// Calculate size of kernel
	sub 	$_kernel_start, %ecx   
	mov		$KERNEL_PHYS_ADDR, %ebx	            // Physical address
	mov		$KERNEL_PHYS_ADDR, %edx	            // Virtual address
	call    _map_address_range

	// Kernel higher half mapping
	mov		$KERNEL_PHYS_ADDR, %ebx	            // Physical address
	mov		$KERNEL_VIRT_ADDR, %edx	            // Virtual address
	// %ecx already contains size of kernel
	call    _map_address_range


	/********************************************
	*               Enable paging               *
	********************************************/
	
	// Set CR3 (pointer to page directory)
	// %eax already contains pointer to page directory
	mov 	%eax, %cr3
	
	// Set paging and protection bits in CR0
	mov		%cr0, %eax
	or	    $(CR0_PE | CR0_PG), %eax	
	mov		%eax, %cr0
	
	// Jump  to higher mapping of kernel
	mov		$_paging_init_higher, %eax
	
_paging_init_higher:

	pop 	%edx
	pop 	%ecx
	pop 	%ebx
	
	// Change return address to higher half kernel
	pop 	%eax
	add 	$KERNEL_OFFSET, %eax
	push	%eax

	ret

/*
 * Set page directory entry to point to page table
 * Inputs:
 *   - %eax: pointer to the page directory
 * 	 - %ebx: pointer to the page table (page aligned)
 *   - %edx: address of an address in the range mapped by this page table
*/
_set_page_dir_entry:
	pusha
	
	// Compute byte offset into the page directory in %edx
	shr		$22, %edx	// NOTE: page directory handles first 12 bits
						// of addres
	shl		$2, %edx	// Each PDE takes up 4 bytes

	// Construct PDE in %ebx
	or		$PDE_FLAG_PRESENT, %ebx // Set present flag
	
	// Set PDE in page directory
	add 	%edx, %eax 	 	// PDE address = page table + offset
	mov		%ebx, (%eax)	// Set PDE
	
	popa
	ret

	
/*
 * Maps a range of physical addresses into the virtual address space
 * Inputs:
 *   - %eax: pointer to the page directory
 * 	 - %ebx: physical address (page aligned)
 *   - %ecx: size of range (bytes)
 * 	 - %edx: virtual address (page aligned)
 * Fail:
 *   Jumps to _init_failure
*/
_map_address_range:
	pusha
	
	// Calculate number of pages to map
	add 	$4095, %ecx 	// Round division by excess
	shr		$12, %ecx		// Divide by 4096
	
	// Iterate over all the pages in the address range
_map_addr_range_lp:
	
	// Map page
	call _map_page
	
	// Calculate next page addresses
	add 	$4096, %ebx 	// Next page physical address
	add 	$4096, %edx		// Next page virtual addressS

	loop  	_map_addr_range_lp

	popa
	ret

/*
 * Maps one page into a virtual address space
 * Inputs:
 * 	 - %eax: Page directory address
 * 	 - %ebx: Page physical address (page aligned)
 *   - %edx: Page virtual address (page aligned)
*/
_map_page:
	pusha
	
	// Calc offset into page directory
	mov		%edx, %ecx
	shr		$22, %ecx	// NOTE: page directory handles first 10 bits
						// of addres
	shl		$2, %ecx	// Each PDE takes up 4 bytes
	add 	%ecx, %eax  // %eax now contains address of correct PDE
	
	// Follow pointer to page table
	mov 	(%eax), %eax
	and		$0xFFFFF000, %eax // Ignore low bits (flags)
	
	// Compute offset in the page table
	and		$0x3FFFFF, %edx	  // Ignore high bits (10 high bits index into the page directory)
	shr		$12, %edx		  // Get index into page table
	shl		$2, %edx		  // Each PTE is 4 bytes
	add		%edx, %eax	 	  // Caluclate address of PTE
	
	// Map page
	or		$PTE_FLAG_PRESENT, %ebx	 // Set present bit
	mov 	%ebx, (%eax)	         // Set PTE
	
	popa
	ret
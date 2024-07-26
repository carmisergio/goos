# Declare constants for the multiboot header.
.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # multiboot magic number
.set CHECKSUM, -(MAGIC + FLAGS) # checksum

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

# Kernel entry point
.section .text
.global _start
.type _start, @function
_start:
	movl $stack_top, %esp
	
	# Push multiboot information to stack
	push %eax
	push %ebx

	# Call global constructors
	call _init

	# Transfer control to the kernel
	call multiboot_entry

	# Hang if kernel_main unexpectedly returns.
	cli
1:	hlt
	jmp 1b
.size _start, . - _start

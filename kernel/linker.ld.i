ENTRY(_start)
SECTIONS
{
 . = 0xC0100000;
 .text BLOCK(4K) : AT(ADDR(.text) - (0xC0100000 - 0x100000))
 {
 _kernel_start = .;
  *(.multiboot)
  *(.text)
 }
 .rodata BLOCK(4K) : AT(ADDR(.rodata) - (0xC0100000 - 0x100000))
 {
  *(.rodata)
 }
 .data BLOCK(4K) : AT(ADDR(.data) - (0xC0100000 - 0x100000))
 {
  *(.data)
 }
 .bss BLOCK(4K) : AT(ADDR(.bss) - (0xC0100000 - 0x100000))
 {
  *(COMMON)
  *(.bss)
 PROVIDE(_kernel_end = .);
 }
 ASSERT((_kernel_end <= (0xC0100000 + (1024 * 4096))), "Kernel exceeds single page table")
}

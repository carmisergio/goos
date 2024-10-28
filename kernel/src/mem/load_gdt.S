#include "mem/const.h"

.section .text
.global load_gdt
    
/*
 * void load_gdt(void *gdt_desc);
 *
 * Sets the GDTR register using the LGDTR instruction and flushes
 * all segment registers
 * #### Parameters:
 *   - void *gdt_desc: pointer to the GDT descriptor
 */
load_gdt:
    push    %ebp
    mov     %esp, %ebp
    
    // Get parameters
    mov     8(%ebp), %eax
    
    // Load GDT
    lgdt    (%eax)
    
    // Flush segment registers

    // Reload CS
    ljmp     $GDT_SEGMENT_KCODE, $load_gdt_reload_cs_done
load_gdt_reload_cs_done:
    
    // Reload data segment registers
    mov     $GDT_SEGMENT_KDATA, %eax
    mov     %eax, %ds
    mov     %eax, %es
    mov     %eax, %fs
    mov     %eax, %gs
    mov     %eax, %ss

    pop     %ebp
    ret
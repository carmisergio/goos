#include "mem/const.h"

.section .text
.global load_idt
    
/*
 * void load_idt(void *idt_pointer);
 *
 * Sets the IDTR register using the IDTR instruction and flushes
 * all segment registers
 * #### Parameters:
 *   - void *idt_pointer: pointer to an IDT pointer structure
 */
load_idt:
    push    %ebp
    mov     %esp, %ebp
    
    // Get parameters
    mov     8(%ebp), %eax
    
    // Load IDT
    lidt    (%eax)
    
    pop     %ebp
    ret
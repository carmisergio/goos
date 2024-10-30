#include "mem/const.h"

// ISR stub for interrupts which do not push an error code to the stack
.macro ISR_VECT_NOERR, n
    .global int_vector_\n
    int_vector_\n:
        pushl $0
        pushl $\n
        jmp int_vector_common
.endm

// ISR stub for interrupts which push an error code to the stack
.macro ISR_VECT_ERR, n
    .global int_vector_\n
    int_vector_\n:
        pushl $\n
        jmp int_vector_common
.endm

// C interrupt handler
.extern interrupt_handler

// Common between all vectors
int_vector_common:

    // Save registers
    push    %eax
    push    %ebx
    push    %ecx
    push    %edx
    push    %esi
    push    %edi
    push    %ebp
    
    // Make sure we're using the kernel data segment
    mov     $GDT_SEGMENT_KDATA, %eax
    mov     %eax, %ds
    mov     %eax, %es
    mov     %eax, %fs
    mov     %eax, %gs

    // Call C interrupt handler
    push    %esp
    call    interrupt_handler
    add     $4, %esp
    
    // Restore registers
    pop     %ebp
    pop     %edi
    pop     %esi
    pop     %edx
    pop     %ecx
    pop     %ebx
    pop     %eax
    
    add     $8, %esp
    iret

////./// Interrupt vectors

// CPU exceptions
// NOTE: commented out are reserved by Intel
ISR_VECT_NOERR 0
ISR_VECT_NOERR 1
//ISR_VECT_NOERR 2
ISR_VECT_NOERR 3
ISR_VECT_NOERR 4
ISR_VECT_NOERR 5
ISR_VECT_NOERR 6
ISR_VECT_NOERR 7
ISR_VECT_ERR   8
ISR_VECT_NOERR 9
ISR_VECT_ERR   10
ISR_VECT_ERR   11
ISR_VECT_ERR   12
ISR_VECT_ERR   13
ISR_VECT_ERR   14
// ISR_VECT_NOERR 15
ISR_VECT_NOERR 16
// ISR_VECT_NOERR 17
// ISR_VECT_NOERR 18
// ISR_VECT_NOERR 19
// ISR_VECT_NOERR 20 
// ISR_VECT_NOERR 21
// ISR_VECT_NOERR 22
// ISR_VECT_NOERR 23
// ISR_VECT_NOERR 24
// ISR_VECT_NOERR 25
// ISR_VECT_NOERR 26
// ISR_VECT_NOERR 27
// ISR_VECT_NOERR 28
// ISR_VECT_NOERR 29
// ISR_VECT_NOERR 30
// ISR_VECT_NOERR 31

// PIC IRQs
ISR_VECT_NOERR 32
ISR_VECT_NOERR 33
ISR_VECT_NOERR 34
ISR_VECT_NOERR 35
ISR_VECT_NOERR 36
ISR_VECT_NOERR 37
ISR_VECT_NOERR 38
ISR_VECT_NOERR 39
ISR_VECT_NOERR 40
ISR_VECT_NOERR 41
ISR_VECT_NOERR 42
ISR_VECT_NOERR 43
ISR_VECT_NOERR 44
ISR_VECT_NOERR 45
ISR_VECT_NOERR 46
ISR_VECT_NOERR 47

// Syscall
ISR_VECT_NOERR 48
    

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

// Common between all vectors
int_vector_common:
    
    add     $8, %esp
    iret

// Interrupt vectors
ISR_VECT_NOERR 0
ISR_VECT_NOERR 1
ISR_VECT_NOERR 2
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
ISR_VECT_NOERR 15
ISR_VECT_NOERR 16
ISR_VECT_NOERR 17
ISR_VECT_NOERR 18
ISR_VECT_NOERR 19
ISR_VECT_NOERR 20 
ISR_VECT_NOERR 21
ISR_VECT_NOERR 22
ISR_VECT_NOERR 23
ISR_VECT_NOERR 24
ISR_VECT_NOERR 25
ISR_VECT_NOERR 26
ISR_VECT_NOERR 27
ISR_VECT_NOERR 28
ISR_VECT_NOERR 29
ISR_VECT_NOERR 30
ISR_VECT_NOERR 31
    

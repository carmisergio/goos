#define _AS 1
#include "proc/ctx.h"    

// Enter userspace (ring3) from kernel mode (ring 0)
//   void go_userspace(proc_ctx_t *ctx)
.section .text
.global go_userspace
go_userspace:
    
    // Get pointer to process context struct passed as argument
    // (skip return address)
    mov 4(%esp), %ebx
    
    // Set up segment registers
    mov PROC_CTX_DS(%ebx), %eax
    mov %eax, %ds
    mov %eax, %es
    mov %eax, %fs
    mov %eax, %gs
    // (SS is set by IRET)
    
    // Push SS
    push %eax // SS
    
    // Push ESP
    mov PROC_CTX_ESP(%ebx), %eax
    push PROC_CTX_ESP(%ebx) // ESP
    //push PROC_CTX_EFLAGS(%ebx)// EFLAGS
    pushf
    push PROC_CTX_CS(%ebx)// CS
    push PROC_CTX_EIP(%ebx) // EIP
    
    // Restore registers state
    mov PROC_CTX_EAX(%ebx), %eax // EAX 
    mov PROC_CTX_ECX(%ebx), %ecx // ECX 
    mov PROC_CTX_EDX(%ebx), %edx // EDX 
    mov PROC_CTX_ESI(%ebx), %esi // ESI 
    mov PROC_CTX_EDI(%ebx), %edi // EDI 
    mov PROC_CTX_EBP(%ebx), %ebp // EBP
    mov PROC_CTX_EBX(%ebx), %ebx // EBX last, be cause it contains
                                 // the pointer to the context

    // To userspace and beyond!
    iret
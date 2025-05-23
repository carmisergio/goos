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
    
    
    // Push SS
    push PROC_CTX_SS(%ebx)
    
    // Push ESP
    push PROC_CTX_ESP(%ebx) // ESP
    push PROC_CTX_EFLAGS(%ebx)// EFLAGS
    // pushf
    push PROC_CTX_CS(%ebx)// CS
    push PROC_CTX_EIP(%ebx) // EIP

    // Push EBX (it will be popped shortly)
    push PROC_CTX_EBX(%ebx)
    
    // Restore registers state
    mov PROC_CTX_EAX(%ebx), %eax // EAX 
    mov PROC_CTX_ECX(%ebx), %ecx // ECX 
    mov PROC_CTX_EDX(%ebx), %edx // EDX 
    mov PROC_CTX_ESI(%ebx), %esi // ESI 
    mov PROC_CTX_EDI(%ebx), %edi // EDI 
    mov PROC_CTX_EBP(%ebx), %ebp // EBP

    // Set segment registers except for DS, since
    // We need it to set up register values
    mov PROC_CTX_DS(%ebx), %ebx
    mov %ebx, %ds
    mov %ebx, %es
    mov %ebx, %fs
    mov %ebx, %gs
    // (SS is set by IRET)
    
    // From now on we cannot directly access data
    pop %ebx // Restore EBX (was pushed to the stack previously)


    // To userspace and beyond!
    iret
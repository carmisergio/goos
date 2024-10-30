#ifndef _SYSCALL_SYSCALL_H
#define _SYSCALL_SYSCALL_H 1

#include "int/interrupts.h"

// Handle system call
void handle_syscall(interrupt_context_t *int_ctx);

#endif
#ifndef _SYSCALL_SYSCALL_H
#define _SYSCALL_SYSCALL_H 1

#include "int/interrupts.h"

// Handle system call
void handle_syscall(interrupt_context_t *int_ctx);

// Trigger dishonorable exit from interrupt context
void dishon_exit_from_int(interrupt_context_t *int_ctx);

// Trigger dishonorable exit from system call context
void dishon_exit_from_syscall();

#endif
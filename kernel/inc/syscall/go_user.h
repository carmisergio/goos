#ifndef _SYSCALL_GO_USER_H
#define _SYSCALL_GO_USER_H 1

#include <stdint.h>

#include "proc/ctx.h"

/*
 * Jump to userspace and start executing a process context
 * #### Parameters:
 *   - ctx: context of the process to run
 */
extern void go_userspace(cpu_ctx_t *ctx);

#endif
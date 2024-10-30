#ifndef _PROC_POC_H
#define _PROC_POC_H 1

#include <stdint.h>

#include "proc/ctx.h"

// Process control block
typedef struct
{
    // CPU Context
    cpu_ctx_t cpu_ctx;
} proc_cb_t;

/*
 * Get current process
 * #### Returns: pointer to the current Process' control block
 */
proc_cb_t *proc_cur();

#endif
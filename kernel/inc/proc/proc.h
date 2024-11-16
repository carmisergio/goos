#ifndef _PROC_POC_H
#define _PROC_POC_H 1

#include <stdint.h>

#include "proc/ctx.h"
#include "mem/vmem.h"

// Process control block
typedef struct proc_cb_s proc_cb_t;
typedef struct proc_cb_s
{
    // Process information
    uint32_t pid;
    proc_cb_t *parent; // Pointer to the partnt process

    // Page directory
    pde_t *pagedir;

    // CPU Context
    cpu_ctx_t cpu_ctx;
};

/*
 * Initialize process management
 */
void proc_init();

/*
 * Add process to the top of the stack
 * Does address space switching
 */
int32_t proc_push();

/*
 * Remove process from the top of the process stack
 * Does address space switching
 */
int32_t proc_pop();

/*
 * Get current process
 * #### Returns: pointer to the current Process' control block
 */
proc_cb_t *proc_cur();

/**
 * Set up CPU context for process execution
 * entry: entrypoint of the process
 */
void proc_setup_cpu_ctx(void *entry);

#endif
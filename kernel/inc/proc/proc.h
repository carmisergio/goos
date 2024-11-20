#ifndef _PROC_POC_H
#define _PROC_POC_H 1

#include <stdint.h>

#include "proc/ctx.h"
#include "mem/vmem.h"
#include "fs/vfs.h"

#define MAX_FILES 16

typedef struct
{
    bool used;

    // Handle of the corresponding VFS file
    vfs_file_handle_t vfs_handle;
} proc_file_t;

// Process control block
typedef struct _proc_cb_t
{
    // Process information
    uint32_t pid;
    struct _proc_cb_t *parent; // Pointer to the partnt process

    // Page directory
    pde_t *pagedir;

    // CPU Context
    cpu_ctx_t cpu_ctx;

    // Current working directory
    char cwd[PATH_MAX + 1];

    // Open files
    proc_file_t files[MAX_FILES];
} proc_cb_t;

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

/*
 * Open file system call
 */
void syscall_open(proc_cb_t *pcb);

/*
 * Close file system call
 */
void syscall_close(proc_cb_t *pcb);

/*
 * Read dir system call
 */
void syscall_readdir(proc_cb_t *pcb);

/*
 * Read system call
 */
void syscall_read(proc_cb_t *pcb);

#endif
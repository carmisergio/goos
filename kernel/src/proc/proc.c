#include "proc/proc.h"

#include <stddef.h>
#include <stdbool.h>
#include "string.h"

#include "sync.h"

#include "config.h"
#include "panic.h"
#include "mem/const.h"
#include "mem/mem.h"
#include "mem/kalloc.h"
#include "cpu.h"
#include "error.h"
#include "log.h"
#include "syscall/syscall.h"
#include "fs/path.h"

#define PROC_STACK_PAGES 4

// Working director of the init process
#define INIT_CWD "0:"

// Configure debugging
#if DEBUG_PROC == 1
#define DEBUG
#endif

// Internal funcitons
static bool alloc_proc_stack(uint32_t n);
static void init_proc_files(proc_file_t files[]);
static bool find_free_file(proc_file_t files[], uint32_t *idx);

// Global objects
proc_cb_t *cur_proc;  // Current process
slock_t cur_proc_lck; // Lock to the current process variable

void proc_init()
{
    // Initialize lock
    slock_init(&cur_proc_lck);

    // Allocate a PCB for the init process
    proc_cb_t *pcb = kalloc(sizeof(proc_cb_t));
    if (!pcb)
        goto fail;

    // Set up PCB of init process
    pcb->pid = 0;                  // Init process has PID 0
    pcb->parent = NULL;            // No parent
    pcb->pagedir = vmem_cur_vas(); // Init process inherits bootstrap VAS
    strcpy(pcb->cwd, INIT_CWD);    // Current working directory
    init_proc_files(pcb->files);

    // Allocate process stack
    if (!alloc_proc_stack(PROC_STACK_PAGES))
        goto fail;

    // Set current process
    cur_proc = pcb;

    return;

fail:
    panic("PROC_INIT_NOMEM", "Out of memory while initializing process management");
}

int32_t proc_push()
{
    int32_t res;

    // Lazily try to get lock
    if (!slock_try_acquire(&cur_proc_lck))
        return E_BUSY;

    // Get reference to current process
    proc_cb_t *parent = cur_proc;

    // Allocate a PCB for the init process
    proc_cb_t *pcb = kalloc(sizeof(proc_cb_t));
    if (!pcb)
    {
        res = E_NOMEM;
        goto fail;
    }

    // Create new VAS for the new process
    pde_t *new_vas = vmem_new_vas();
    if (!new_vas)
    {
        res = E_NOMEM;
        goto fail_free_pcb;
    }

    // Switch to the new address space
    vmem_switch_vas(new_vas);

    // Allocate process stack
    // (inside new VAS)
    if (!alloc_proc_stack(PROC_STACK_PAGES))
    {
        res = E_NOMEM;
        goto fail_delete_vas;
    }

    // Set up PCB
    pcb->pid = parent->pid + 1;
    pcb->parent = parent;
    pcb->pagedir = new_vas;
    strcpy(pcb->cwd, parent->cwd); // Inherits parent CWD
    init_proc_files(pcb->files);

    // Set current process
    cur_proc = pcb;

#ifdef DEBUG
    kprintf("[PROC] New process: PID = %u\n", pcb->pid);
#endif

    slock_release(&cur_proc_lck);
    return 0;

fail_delete_vas:
    vmem_delete_vas(new_vas);
fail_free_pcb:
    kfree(pcb);
fail:
    slock_release(&cur_proc_lck);
    return res;
}

int32_t proc_pop()
{
    int res;

    // Lazily try to get lock
    if (!slock_try_acquire(&cur_proc_lck))
        return E_BUSY;

    proc_cb_t *pcb = cur_proc;
    proc_cb_t *parent_pcb = pcb->parent;

#ifdef DEBUG
    kprintf("[PROC] Destroy process: PID = %u\n", pcb->pid);
#endif

    // The init process (parent == NULL) is not allowd to exit
    if (!parent_pcb)
    {
        res = E_NOTPERM;
        goto fail;
    }

    // Free userspace memory for the current process
    vmem_destroy_uvas();

    // Switch to the parent address space and delete
    // the current processes'
    vmem_switch_vas(parent_pcb->pagedir);
    vmem_delete_vas(pcb->pagedir);

    // The parent process is now the current process
    cur_proc = parent_pcb;

    // Free PCB
    kfree(pcb);

    slock_release(&cur_proc_lck);

    return 0;
fail:
    slock_release(&cur_proc_lck);
    return res;
}

proc_cb_t *proc_cur()
{
    return cur_proc;
}

// Set up CPU context for process execution
// entry: entrypoint of the program
void proc_setup_cpu_ctx(void *entry)
{
    cpu_ctx_t *ctx = &cur_proc->cpu_ctx;

    // General registers
    ctx->eax = 0;
    ctx->ebx = 0;
    ctx->ecx = 0;
    ctx->edx = 0;
    ctx->esi = 0;
    ctx->edi = 0;

    // Segment selectors
    ctx->ds = GDT_SEGMENT_UDATA | SEGSEL_USER;
    ctx->cs = GDT_SEGMENT_UCODE | SEGSEL_USER;
    ctx->ss = GDT_SEGMENT_UDATA | SEGSEL_USER;

    // Flags
    ctx->eflags = EFLAGS | EFLAGS_IF;

    // Stack
    // The stack grows down from the top of the UVAS
    ctx->esp = KERNEL_VAS_START;
    ctx->ebp = ctx->esp; // Shouldn't be necessary

    // Instruction pointer
    ctx->eip = (uint32_t)entry;
}

// Open file system call
void syscall_open(proc_cb_t *pcb)
{
    int32_t res;

    // Get parameters
    const char *p_path = (const char *)pcb->cpu_ctx.ebx;
    uint32_t p_path_n = pcb->cpu_ctx.ecx;
    fopts p_fopts = pcb->cpu_ctx.edx;

    // Validate pointers
    if (!vmem_validate_user_ptr_mapped(p_path, p_path_n))
    {
        dishon_exit_from_syscall();
        return;
    }

    // Check length of path
    if (p_path_n > PATH_MAX)
    {
        res = E_INVREQ;
        goto fail;
    }

    // Copy path to kernel memory
    char path[PATH_MAX + 1];
    memcpy(path, p_path, p_path_n);
    path[p_path_n] = 0;

    // Resolve relative path
    char abspath[PATH_MAX + 1];
    if (!path_resolve_relative(abspath, pcb->cwd, path))
    {
        res = E_NOENT;
        goto fail;
    }

    // Find a file slot in the process file list
    uint32_t file_handle;
    if (!find_free_file(pcb->files, &file_handle))
    {
        res = E_TOOMANY;
        goto fail;
    }
    proc_file_t *file = &pcb->files[file_handle];

    // Open file
    vfs_file_handle_t vfs_file;
    if ((vfs_file = vfs_open(abspath, p_fopts)) < 0)
    {
        res = vfs_file; // Error is in vfs_file
        goto fail;
    }

    // Save file in files list
    file->vfs_handle = vfs_file;
    file->used = true;

    // Return file descriptor
    res = file_handle;

fail:
    pcb->cpu_ctx.eax = (uint32_t)res;
}

// Close file system call
void syscall_close(proc_cb_t *pcb)
{
    int32_t res;

    // Get parameters
    uint32_t p_fd = pcb->cpu_ctx.ebx;

    // Check if file is in use
    if (p_fd >= MAX_FILES || !pcb->files[p_fd].used)
    {
        res = E_NOENT;
        goto fail;
    }
    proc_file_t *file = &pcb->files[p_fd];

    // Close file
    vfs_close(file->vfs_handle);

    file->used = false;

    res = 0;
fail:
    pcb->cpu_ctx.eax = (uint32_t)res;
}

// Read dir system call
typedef struct __attribute__((packed))
{
    uint32_t fd;
    dirent_t *buf;
    uint32_t offset, n;
} sc_readdir_params_t;
void syscall_readdir(proc_cb_t *pcb)
{
    int32_t res;

    // Get parameters
    sc_readdir_params_t *params = (sc_readdir_params_t *)pcb->cpu_ctx.ebx;

    // Validate parameters struct
    if (!vmem_validate_user_ptr_mapped(params, sizeof(sc_readdir_params_t)))
    {
        dishon_exit_from_syscall();
        return;
    }

    // Check if file is in use
    if (params->fd >= MAX_FILES || !pcb->files[params->fd].used)
    {
        res = E_NOENT;
        goto fail;
    }

    // Validate buffer
    if (!vmem_validate_user_ptr_mapped(params->buf, params->n * sizeof(dirent_t)))
    {
        dishon_exit_from_syscall();
        return;
    }

    // Execute readdir operation
    res = vfs_readdir(pcb->files[params->fd].vfs_handle, params->buf, params->offset, params->n);

fail:
    pcb->cpu_ctx.eax = (uint32_t)res;
}

// Read system call
typedef struct __attribute__((packed))
{
    uint32_t fd;
    uint8_t *buf;
    uint32_t offset, n;
} sc_read_params_t;
void syscall_read(proc_cb_t *pcb)
{
    int32_t res;

    // Get parameters
    sc_read_params_t *params = (sc_read_params_t *)pcb->cpu_ctx.ebx;

    // Validate parameters struct
    if (!vmem_validate_user_ptr_mapped(params, sizeof(sc_read_params_t)))
    {
        dishon_exit_from_syscall();
        return;
    }

    // Check if file is in use
    if (params->fd >= MAX_FILES || !pcb->files[params->fd].used)
    {
        res = E_NOENT;
        goto fail;
    }

    // Validate buffer
    if (!vmem_validate_user_ptr_mapped(params->buf, params->n))
    {
        dishon_exit_from_syscall();
        return;
    }

    // Execute readdir operation
    res = vfs_read(pcb->files[params->fd].vfs_handle, params->buf, params->offset, params->n);

fail:
    pcb->cpu_ctx.eax = (uint32_t)res;
}

static bool alloc_proc_stack(uint32_t npages)
{
    // Allocate n pages before the kvas
    return mem_make_avail(
        (void *)(KERNEL_VAS_START - MEM_PAGE_SIZE * npages), npages);
}

// Initialize a process' file array with no used files
static void init_proc_files(proc_file_t files[])
{
    for (size_t i = 0; i < MAX_FILES; i++)
        files[i].used = false;
}

// Find unused file in a process' file array
static bool find_free_file(proc_file_t files[], uint32_t *idx)
{
    for (size_t i = 0; i < MAX_FILES; i++)
    {
        if (!files[i].used)
        {
            *idx = i;
            return true;
        }
    }
    return false;
}
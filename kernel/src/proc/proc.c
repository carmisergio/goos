#include "proc/proc.h"

#include <stddef.h>
#include <stdbool.h>

#include "sync.h"

#include "config.h"
#include "panic.h"
#include "mem/const.h"
#include "mem/mem.h"
#include "mem/kalloc.h"
#include "cpu.h"
#include "error.h"
#include "log.h"

#define PROC_STACK_PAGES 4

// Configure debugging
#if DEBUG_PROC == 1
#define DEBUG
#endif

// Internal funcitons
bool alloc_proc_stack(uint32_t n);

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

    // Set up PCB
    pcb->pid = 0;                  // Init process has PID 0
    pcb->parent = NULL;            // No parent
    pcb->pagedir = vmem_cur_vas(); // Init process inherits bootstrap VAS

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

bool alloc_proc_stack(uint32_t npages)
{
    // Allocate n pages before the kvas
    return mem_make_avail(
        (void *)(KERNEL_VAS_START - MEM_PAGE_SIZE * npages), npages);
}
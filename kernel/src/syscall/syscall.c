#include "syscall/syscall.h"

#include <string.h>
#include "mini-printf.h"

#include "config.h"
#include "syscall/go_user.h"
#include "proc/proc.h"
#include "log.h"
#include "mem/const.h"
#include "mem/vmem.h"
#include "boot/boot.h"
#include "panic.h"
#include "fs/vfs.h"
#include "proc/elf.h"
#include "error.h"
#include "mem/kalloc.h"
#include "fs/path.h"

#include "clock.h"
#include "console/console.h"

#define MSG_N 64

// Configure debugging
#if DEBUG_SYSCALL == 1
#define DEBUG
#endif

/*
 * SYSCALL TABLE
 * == 0x01XX ---- Clock
 *  - 0x0100: Get system time
 *  - 0x0101: Get local time
 *  - 0x0110: Delay (ms)
 */

// System call numbers
typedef enum
{
    // Clock system calls
    // SYSCALL_GET_SYSTEM_TIME = 0x0100,
    SYSCALL_GET_LOCAL_TIME = 0x0101,
    SYSCALL_DELAY_MS = 0x0110,

    // Console system calls
    SYSCALL_CONSOLE_WRITE = 0x0200,
    SYSCALL_CONSOLE_READLINE = 0x0201,
    SYSCALL_CONSOLE_GETCHAR = 0x0202,

    // Process management system calls
    SYSCALL_EXIT = 0x1000,
    SYSCALL_EXEC = 0x1001,
    SYSCALL_CHANGE_CWD = 0x1002,
    SYSCALL_GET_CWD = 0x1003,

    // Filesystem syscalls
    SYSCALL_MOUNT = 0x1100,
    SYSCALL_UNMOUNT = 0x1101,
    SYSCALL_OPEN = 0x1110,
    SYSCALL_CLOSE = 0x1111,
    SYSCALL_READ = 0x1112,
    // SYSCALL_WRITE = 0x1113,
    SYSCALL_READDIR = 0x1114,
} syscall_n_t;

void iret_to_kernel(interrupt_context_t *int_ctx, void *dst);
void syscall_handler();
void syscall_dummy();
void syscall_get_local_time(proc_cb_t *pcb);
void syscall_delay_ms(proc_cb_t *pcb);
void syscall_console_write(proc_cb_t *pcb);
void syscall_console_readline(proc_cb_t *pcb);
void syscall_console_getchar(proc_cb_t *pcb);
void syscall_exit(proc_cb_t *pcb);
void syscall_exec(proc_cb_t *pcb);
void syscall_change_cwd(proc_cb_t *pcb);
void syscall_mount(proc_cb_t *pcb);
void syscall_unmount(proc_cb_t *pcb);
void syscall_get_cwd(proc_cb_t *pcb);
void dishonorable_exit_handler();

// This function is executed in the intererupt handler of the
// system call interrupt. It copies the current CPU context into the
// Process Control Block, and then transfer control to the actual system call
// dispatch system by modifying the interrupt return frame
void handle_syscall(interrupt_context_t *int_ctx)
{
    iret_to_kernel(int_ctx, syscall_handler);
}

void dishon_exit_from_int(interrupt_context_t *int_ctx)
{
    iret_to_kernel(int_ctx, dishonorable_exit_handler);
}

// Handle a dishonorable exit from a process
// This function is executed when a process tries to do womething
// it shouldn't, and has to be immediately terminated
// The parent process will receive E_TERM as the exec() call result
void dishon_exit_from_syscall()
{
    char msg[MSG_N];
    int32_t res;

    // Exit from current process
    if ((res = proc_pop()) < 0)
    {
        // Failure

        // There's nothing we can do but panic
        snprintf(msg, MSG_N, "Error in dishonorable exit handler: %s\n",
                 error_get_message(res));
        panic("DISHONORABLE_EXIT_ERR", msg);

        // Will never reach here
    }

    // Recover new current proces
    proc_cb_t *pcb = proc_cur();

    // This will appear to the parent process as the result from the
    // exec() call
    pcb->cpu_ctx.eax = 0;              // Because the exit call was succesful
    pcb->cpu_ctx.ebx = (uint32_t)-100; // TODO: define process return codes
}

// Return from interrupt to kernel mode
// It is used for system calls and exception or keyboard interrupt
// triggred unhonorable exits
// dst: pointer to jump to
void iret_to_kernel(interrupt_context_t *int_ctx, void *dst)
{
    // Save process CPU context
    proc_cb_t *pcb = proc_cur();
    pcb->cpu_ctx.eax = int_ctx->eax;
    pcb->cpu_ctx.ebx = int_ctx->ebx;
    pcb->cpu_ctx.ecx = int_ctx->ecx;
    pcb->cpu_ctx.edx = int_ctx->edx;
    pcb->cpu_ctx.esi = int_ctx->esi;
    pcb->cpu_ctx.edi = int_ctx->edi;
    pcb->cpu_ctx.eip = int_ctx->eip;
    pcb->cpu_ctx.eflags = int_ctx->eflags;
    pcb->cpu_ctx.esp = int_ctx->esp;
    pcb->cpu_ctx.ebp = int_ctx->ebp;

    // Jump to syscall handler
    int_ctx->eip = (uint32_t)dst;
    int_ctx->cs = GDT_SEGMENT_KCODE;
    int_ctx->ss = GDT_SEGMENT_KDATA;
    int_ctx->ds = GDT_SEGMENT_KDATA;
    int_ctx->es = GDT_SEGMENT_KDATA;
    int_ctx->fs = GDT_SEGMENT_KDATA;
    int_ctx->gs = GDT_SEGMENT_KDATA;
    int_ctx->esp = (uint32_t)&kernel_stack_top;
    int_ctx->ebp = (uint32_t)&kernel_stack_top;
}

// Actual system call handler, executed OUTSIDE of the interrupt
// service routine context
void syscall_handler()
{
    proc_cb_t *pcb = proc_cur();

    // Syscall number is in EAX
    syscall_n_t syscall_n = pcb->cpu_ctx.eax;

#ifdef DEBUG
    kprintf("[SYSCALL] %d\n", syscall_n);
#endif

    // Test protected instructions

    // Dispatch handler
    switch (syscall_n)
    {
        // Clock syscalls
    case SYSCALL_GET_LOCAL_TIME:
        syscall_get_local_time(pcb);
        break;
    case SYSCALL_DELAY_MS:
        syscall_delay_ms(pcb);
        break;

        // Console syscalls
    case SYSCALL_CONSOLE_WRITE:
        syscall_console_write(pcb);
        break;
    case SYSCALL_CONSOLE_READLINE:
        syscall_console_readline(pcb);
        break;
    case SYSCALL_CONSOLE_GETCHAR:
        syscall_console_getchar(pcb);
        break;

        // Process management syscalls
    case SYSCALL_EXIT:
        syscall_exit(pcb);
        break;
    case SYSCALL_EXEC:
        syscall_exec(pcb);
        break;
    case SYSCALL_CHANGE_CWD:
        syscall_change_cwd(pcb);
        break;
    case SYSCALL_GET_CWD:
        syscall_get_cwd(pcb);
        break;

        // Filesystem system calls
    case SYSCALL_MOUNT:
        syscall_mount(pcb);
        break;
    case SYSCALL_UNMOUNT:
        syscall_unmount(pcb);
        break;
    case SYSCALL_OPEN:
        syscall_open(pcb);
        break;
    case SYSCALL_CLOSE:
        syscall_close(pcb);
        break;
    case SYSCALL_READ:
        syscall_read(pcb);
        break;
    case SYSCALL_READDIR:
        syscall_readdir(pcb);
        break;

    default:
        // Unknown system call
        // Terminate user process
        dishon_exit_from_syscall();
    }

    // Recover new current process, as it could have been changed
    // by the system call
    pcb = proc_cur();

    // Return to process
    go_userspace(&pcb->cpu_ctx);
}

// Gets the local time in seconds
void syscall_get_local_time(proc_cb_t *pcb)
{
    pcb->cpu_ctx.eax = clock_get_local();
}

// Delay Milliseconds syscall
// Waits n milliseconds before returning control to the program
void syscall_delay_ms(proc_cb_t *pcb)
{
    // Get parameters
    uint32_t time = pcb->cpu_ctx.ebx;

    clock_delay_ms(time);
}

// Console write syscall
void syscall_console_write(proc_cb_t *pcb)
{

    // Get parameters
    char *s = (char *)pcb->cpu_ctx.ebx;
    uint32_t n = pcb->cpu_ctx.ecx;

    // Check string pointer
    if (!vmem_validate_user_ptr_mapped(s, n))
    {
        dishon_exit_from_syscall();
        return;
    }

    console_write(s, n);
}

// Console readline syscall
void syscall_console_readline(proc_cb_t *pcb)
{
    // Get parameters
    char *buf = (char *)pcb->cpu_ctx.ebx;
    uint32_t n = pcb->cpu_ctx.ecx;

    // Check string pointer
    if (!vmem_validate_user_ptr_mapped(buf, n))
    {
        dishon_exit_from_syscall();
        return;
    }

    int32_t res = console_readline(buf, n);

    // Set return value
    pcb->cpu_ctx.eax = res;
}

// Console getchar syscall
void syscall_console_getchar(proc_cb_t *pcb)
{
    uint32_t res = (uint32_t)_g_console_getchar();

    // Set return value
    pcb->cpu_ctx.eax = res;
}

// Exit syscall
void syscall_exit(proc_cb_t *pcb)
{
    int32_t retval = pcb->cpu_ctx.ebx;
    int32_t res;

    set_terminate_lock();

    // Exit from current process
    if ((res = proc_pop()) < 0)
    {
        // Failure
        // Return to current process

        release_terminate_lock();

        // Return value to caller process
        pcb->cpu_ctx.eax = (uint32_t)res;
        return;
    }

    // Recover new current proces
    pcb = proc_cur();

    release_terminate_lock();

    // This will appear to the parent process as the result from the
    // exec() call
    pcb->cpu_ctx.eax = 0;
    pcb->cpu_ctx.ebx = (uint32_t)retval;
}

// Exec syscall
void syscall_exec(proc_cb_t *pcb)
{
    int32_t res;

    // Get parameters
    char *p_path = (char *)pcb->cpu_ctx.ebx;
    uint32_t p_n = pcb->cpu_ctx.ecx;

    // Validate path pointer
    if (!vmem_validate_user_ptr_mapped(p_path, p_n))
    {
        dishon_exit_from_syscall();
        return;
    }

    // Check length of path
    if (p_n > PATH_MAX)
    {
        res = E_INVREQ;
        goto fail;
    }

    // Copy pointer to kernel memory
    char path[PATH_MAX + 1];
    memcpy(path, p_path, p_n);
    path[p_n] = 0;

    // Resolve relative path
    char abspath[PATH_MAX + 1];
    if (!path_resolve_relative(abspath, pcb->cwd, path))
    {
        res = E_NOENT;
        goto fail;
    }

    set_terminate_lock();

    // Open file
    vfs_file_handle_t file;
    if ((file = vfs_open(abspath, 0)) < 0)
    {
        res = file;
        goto fail;
    }

    kprintf("[SYSCALL] Exec: file opened: %s\n", abspath);

    // Create new process
    if ((res = proc_push()) < 0)
    {
        // Failure
        // Return to current process
        goto fail_closefile;
    }

    // NOTE: We are now in the context of the new process

    // Load ELF
    void *entry;
    if ((res = elf_load(file, &entry)) < 0)
        goto fail_destroyproc;

    // Close executable file
    vfs_close(file);

    // Here would be where we set up the args and other things
    // Passed to the new process

    // Set up process CPU context
    proc_setup_cpu_ctx(entry);

    release_terminate_lock();

    return;
fail_closefile:
    vfs_close(file);
fail_destroyproc:
    proc_pop();
fail:
    release_terminate_lock();
    // Return value to caller process
    pcb->cpu_ctx.eax = (uint32_t)res;
}

// Change current working directory syscall
void syscall_change_cwd(proc_cb_t *pcb)
{
    int32_t res;

    // Get parameters
    char *p_path = (char *)pcb->cpu_ctx.ebx;
    uint32_t p_n = pcb->cpu_ctx.ecx;

    // Validate path pointer
    if (!vmem_validate_user_ptr_mapped(p_path, p_n))
    {
        dishon_exit_from_syscall();
        return;
    }

    // Check length of path
    if (p_n > PATH_MAX)
    {
        res = E_INVREQ;
        goto fail;
    }

    // Copy relative path to kernel memory
    char relpath[PATH_MAX + 1];
    memcpy(relpath, p_path, p_n);
    relpath[p_n] = 0;

    // Resolve relative path
    char abspath[PATH_MAX + 1];
    if (!path_resolve_relative(abspath, pcb->cwd, relpath))
    {
        res = E_NOENT;
        goto fail;
    }

    set_terminate_lock();

    // Check if directory exists
    int32_t file;
    if ((file = vfs_open(abspath, FOPT_DIR)) < 0)
    {
        res = file;
        goto fail;
    }
    vfs_close(file);

    // Change current working directory of the process
    strcpy(pcb->cwd, abspath);

    // Success!
    res = 0;

fail:
    release_terminate_lock();
    // Set result
    pcb->cpu_ctx.eax = res;
}

// Get current working directory of a process
void syscall_get_cwd(proc_cb_t *pcb)
{
    int32_t res;

    // Get parameters
    char *p_buf = (char *)pcb->cpu_ctx.ebx;
    // The length of the buffer is implied to be PATH_MAX + 1

    // Validate buffer pointer
    if (!vmem_validate_user_ptr_mapped(p_buf, PATH_MAX + 1))
    {
        dishon_exit_from_syscall();
        return;
    }

    // Get current working directory of process
    strcpy(p_buf, pcb->cwd);

    // Success!
    res = 0;

fail:
    // Set result
    pcb->cpu_ctx.eax = res;
}

// Mount syscall
typedef struct __attribute__((packed))
{
    uint32_t mp;
    char *blkdev, *fs_type;
    uint32_t blkdev_n, fs_type_n;
} sc_mount_params_t;
void syscall_mount(proc_cb_t *pcb)
{
    int32_t res;

    // Get parameters
    sc_mount_params_t *params = (sc_mount_params_t *)pcb->cpu_ctx.ebx;

    // Validate parameters struct
    if (!vmem_validate_user_ptr_mapped(params, sizeof(sc_mount_params_t)))
    {
        dishon_exit_from_syscall();
        return;
    }

    // Validate strings
    if (!vmem_validate_user_ptr_mapped(params->blkdev, params->blkdev_n) ||
        !vmem_validate_user_ptr_mapped(params->fs_type, params->fs_type_n))
    {
        dishon_exit_from_syscall();
        return;
    }

    // Check length of path
    if (params->blkdev_n > BLKDEV_MAX || params->fs_type_n > FS_TYPE_MAX)
    {
        res = E_INVREQ;
        goto fail;
    }

    // Copy strings to kernel memroy
    char blkdev[BLKDEV_MAX + 1], fs_type[FS_TYPE_MAX + 1];
    memcpy(blkdev, params->blkdev, params->blkdev_n);
    memcpy(fs_type, params->fs_type, params->fs_type_n);
    blkdev[params->blkdev_n] = 0;
    fs_type[params->fs_type_n] = 0;

    set_terminate_lock();

    // Do mount
    if ((res = vfs_mount(blkdev, params->mp, fs_type)) < 0)
        goto fail;

    // Success!
    res = 0;

fail:
    release_terminate_lock();
    // Set result
    pcb->cpu_ctx.eax = res;
}

// Unmount syscall
void syscall_unmount(proc_cb_t *pcb)
{
    int32_t res;

    // Get parameters
    uint32_t mp = pcb->cpu_ctx.ebx;

    set_terminate_lock();

    // Do unmount
    if ((res = vfs_unmount(mp)) < 0)
        goto fail;

    // Success!
    res = 0;

fail:
    release_terminate_lock();
    // Set result
    pcb->cpu_ctx.eax = res;
}

// Called by handle_dishonoraable_exit, not syscall
void dishonorable_exit_handler()
{
    dishon_exit_from_syscall();

    // Return to process
    proc_cb_t *pcb = proc_cur();
    go_userspace(&pcb->cpu_ctx);
}

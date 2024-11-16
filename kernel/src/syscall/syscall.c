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
    SYSCALL_GET_SYSTEM_TIME = 0x0100,
    SYSCALL_GET_LOCAL_TIME = 0x0101,
    SYSCALL_DELAY_MS = 0x0110,

    // Console system calls
    SYSCALL_CONSOLE_WRITE = 0x0200,
    SYSCALL_CONSOLE_READLINE = 0x0201,

    // Process management system calls
    SYSCALL_EXIT = 0x1000,
    SYSCALL_EXEC = 0x1001,
} syscall_n_t;

void iret_to_kernel(interrupt_context_t *int_ctx, void *dst);
void syscall_handler();
void syscall_dummy();
void syscall_delay_ms(uint32_t time);
void syscall_console_write(char *s, size_t n);
int32_t syscall_console_readline(char *buf, size_t n);
void syscall_exit(proc_cb_t *pcb);
void syscall_exec(proc_cb_t *pcb);
void dishonorable_exit_handler();
void do_dishonorable_exit();

// This function is executed in the intererupt handler of the
// system call interrupt. It copies the current CPU context into the
// Process Control Block, and then transfer control to the actual system call
// dispatch system by modifying the interrupt return frame
void handle_syscall(interrupt_context_t *int_ctx)
{
    iret_to_kernel(int_ctx, syscall_handler);
}

void handle_dishonorable_exit(interrupt_context_t *int_ctx)
{
    iret_to_kernel(int_ctx, dishonorable_exit_handler);
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
    case SYSCALL_DELAY_MS:
        syscall_delay_ms(pcb->cpu_ctx.ebx);
        break;

        // Console syscalls
    case SYSCALL_CONSOLE_WRITE:
        syscall_console_write((char *)pcb->cpu_ctx.ebx, pcb->cpu_ctx.ecx);
        break;
    case SYSCALL_CONSOLE_READLINE:
        pcb->cpu_ctx.eax = syscall_console_readline((char *)pcb->cpu_ctx.ebx,
                                                    pcb->cpu_ctx.ecx);
        break;

        // Process management syscalls
    case SYSCALL_EXIT:
        syscall_exit(pcb);
        break;
    case SYSCALL_EXEC:
        syscall_exec(pcb);
        break;

    default:
        // Unknown system call
        // Terminate user process
        do_dishonorable_exit();
    }

    // Recover new current process, as it could have been changed
    // by the system call
    pcb = proc_cur();

    // Return to process
    go_userspace(&pcb->cpu_ctx);
}

// Delay Milliseconds syscall
// Waits n milliseconds before returning control to the program
void syscall_delay_ms(uint32_t time)
{
    clock_delay_ms(time);
}

// Console write syscall
void syscall_console_write(char *s, size_t n)
{
    // Check string pointer
    if (!vmem_validate_user_ptr_mapped(s, n))
    {
        do_dishonorable_exit();
        return;
    }

    console_write(s, n);
}

// Console readline syscall
int32_t syscall_console_readline(char *buf, size_t n)
{
    // Check string pointer
    if (!vmem_validate_user_ptr(buf, n))
    {
        do_dishonorable_exit();
        return -1;
    }

    return console_readline(buf, n);
}

// Exit syscall
void syscall_exit(proc_cb_t *pcb)
{
    int32_t retval = pcb->cpu_ctx.ebx;
    int32_t res;

    // Exit from current process
    if ((res = proc_pop()) < 0)
    {
        // Failure
        // Return to current process

        // Return value to caller process
        pcb->cpu_ctx.eax = (uint32_t)res;
        return;
    }

    // Recover new current proces
    pcb = proc_cur();

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
    if (!vmem_validate_user_ptr(p_path, p_n))
    {
        do_dishonorable_exit();
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

    // Open file
    vfs_file_handle_t file;
    if ((file = vfs_open(path, 0)) < 0)
    {
        res = file;
        goto fail;
    }

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

    return;
fail_closefile:
    vfs_close(file);
fail_destroyproc:
    proc_pop();
fail:
    // Return value to caller process
    pcb->cpu_ctx.eax = (uint32_t)res;
}

// Called by handle_dishonoraable_exit, not syscall
void dishonorable_exit_handler()
{
    do_dishonorable_exit();

    // Return to process
    proc_cb_t *pcb = proc_cur();
    go_userspace(&pcb->cpu_ctx);
}

// Handle a dishonorable exit from a process
// This function is executed when a process tries to do womething
// it shouldn't, and has to be immediately terminated
// The parent process will receive E_TERM as the exec() call result
void do_dishonorable_exit()
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
    pcb->cpu_ctx.eax = 0; // Because the exit call was succesful
    pcb->cpu_ctx.ebx = (uint32_t)E_TERM;
}

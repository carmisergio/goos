#include "syscall/syscall.h"

#include "mini-printf.h"

#include "syscall/go_user.h"
#include "proc/proc.h"
#include "log.h"
#include "mem/const.h"
#include "boot/boot.h"
#include "panic.h"

#include "clock.h"
#include "console/console.h"

#define MSG_N 64

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
} syscall_n_t;

void syscall_handler();
void syscall_delay_ms(uint32_t time);
void syscall_console_write(char *s, size_t n);

// This function is executed in the intererupt handler of the
// system call interrupt. It copies the current CPU context into the
// Process Control Block, and then transfer control to the actual system call
// dispatch system by modifying the interrupt return frame
void handle_syscall(interrupt_context_t *int_ctx)
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
    int_ctx->eip = (uint32_t)&syscall_handler;
    int_ctx->cs = GDT_SEGMENT_KCODE;
    int_ctx->ss = GDT_SEGMENT_KDATA;
    int_ctx->esp = (uint32_t)&kernel_stack_top;
    int_ctx->ebp = (uint32_t)&kernel_stack_top;
}

// Actual system call handler, executed OUTSIDE of the interrupt
// service routine context
void syscall_handler()
{
    char msg[MSG_N];
    proc_cb_t *pcb = proc_cur();

    // Syscall number is in EAX
    syscall_n_t syscall_n = pcb->cpu_ctx.eax;

    // Dispatch handler
    switch (syscall_n)
    {
    case 0:
        kprintf("HANDLING SYSCALL\n");
        break;
        // Clock syscalls
    case SYSCALL_DELAY_MS:
        syscall_delay_ms(pcb->cpu_ctx.ebx);
        break;

        // Console syscalls
    case SYSCALL_CONSOLE_WRITE:
        syscall_console_write((char *)pcb->cpu_ctx.ebx, pcb->cpu_ctx.ecx);
        break;

    default:
        mini_snprintf(msg, MSG_N, "Unknown system call: %d\n", syscall_n);
        panic("SYSCALL_UNKNOWN", msg);
    }

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
    console_write(s, n);
}
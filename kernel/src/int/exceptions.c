#include "int/exceptions.h"

#include <stdint.h>
#include "mini-printf.h"

#include "log.h"
#include "panic.h"
#include "mem/vmem.h"
#include "syscall/syscall.h"

#define PANIC_MSG_BUF_MAX 256

// Read value of CR2
static inline uint32_t get_cr2_value()
{
    uint32_t cr2;
    __asm__("mov %%cr2, %0" : "=r"(cr2));
    return cr2;
}

void handle_exception(interrupt_context_t *ctx)
{
    char msg_buf[PANIC_MSG_BUF_MAX];

    // If the exception was triggered in a user context,
    // invoke the unhonorable exit handler of the current process
    // If EIP is in the user VAS, then the offending instruction
    // has to be a user program instruction
    if (vmem_validate_user_ptr((void *)ctx->eip, 1))
    {
        kprintf("[PROC] Exception %u\n", ctx->vec);
        handle_dishonorable_exit(ctx);
        return;
    }

    switch (ctx->vec)
    {
    case 0:
        // Division by 0
        mini_snprintf(msg_buf, PANIC_MSG_BUF_MAX,
                      "Division by 0 exception \n\n"
                      "Context:\n"
                      " EAX: %u\n"
                      " EBX: %u\n"
                      " ECX: %u\n"
                      " EDX: %u\n"
                      " ESI: 0x%x\n"
                      " EDI: 0x%x\n"
                      " EFLAGS: 0x%x\n"
                      " EIP: 0x%x\n"
                      " CS: 0x%x\n",
                      ctx->eax,
                      ctx->ebx,
                      ctx->ecx,
                      ctx->edx,
                      ctx->esi,
                      ctx->edi,
                      ctx->eip,
                      ctx->cs);
        panic("EXCPT_DIV0", msg_buf);
        break;

    case 1:
        // Debug
        mini_snprintf(msg_buf, PANIC_MSG_BUF_MAX,
                      "INT 1: Debug exception \n\n"
                      "Context:\n"
                      " EAX: %u\n"
                      " EBX: %u\n"
                      " ECX: %u\n"
                      " EDX: %u\n"
                      " ESI: 0x%x\n"
                      " EDI: 0x%x\n"
                      " EFLAGS: 0x%x\n"
                      " EIP: 0x%x\n"
                      " CS: 0x%x\n",
                      ctx->eax,
                      ctx->ebx,
                      ctx->ecx,
                      ctx->edx,
                      ctx->esi,
                      ctx->edi,
                      ctx->eip,
                      ctx->cs);
        panic("EXCPT_DEBUG", msg_buf);
        break;

    case 3:
        // Breakpoint
        mini_snprintf(msg_buf, PANIC_MSG_BUF_MAX,
                      "INT 3: Breakpoint exception \n\n"
                      "Context:\n"
                      " EAX: %u\n"
                      " EBX: %u\n"
                      " ECX: %u\n"
                      " EDX: %u\n"
                      " ESI: 0x%x\n"
                      " EDI: 0x%x\n"
                      " EFLAGS: 0x%x\n"
                      " EIP: 0x%x\n"
                      " CS: 0x%x\n",
                      ctx->eax,
                      ctx->ebx,
                      ctx->ecx,
                      ctx->edx,
                      ctx->esi,
                      ctx->edi,
                      ctx->eip,
                      ctx->cs);
        panic("EXCPT_BREAKPOINT", msg_buf);
        break;

    case 4:
        // Overflow
        mini_snprintf(msg_buf, PANIC_MSG_BUF_MAX,
                      "INT 4: Overflow exception \n\n"
                      "Context:\n"
                      " EAX: %u\n"
                      " EBX: %u\n"
                      " ECX: %u\n"
                      " EDX: %u\n"
                      " ESI: 0x%x\n"
                      " EDI: 0x%x\n"
                      " EFLAGS: 0x%x\n"
                      " EIP: 0x%x\n"
                      " CS: 0x%x\n",
                      ctx->eax,
                      ctx->ebx,
                      ctx->ecx,
                      ctx->edx,
                      ctx->esi,
                      ctx->edi,
                      ctx->eip,
                      ctx->cs);
        panic("EXCPT_OVERFLOW", msg_buf);
        break;

    case 5:
        // Bound Check
        mini_snprintf(msg_buf, PANIC_MSG_BUF_MAX,
                      "INT 5: Bounds Check exception \n\n"
                      "Context:\n"
                      " EAX: %u\n"
                      " EBX: %u\n"
                      " ECX: %u\n"
                      " EDX: %u\n"
                      " ESI: 0x%x\n"
                      " EDI: 0x%x\n"
                      " EFLAGS: 0x%x\n"
                      " EIP: 0x%x\n"
                      " CS: 0x%x\n",
                      ctx->eax,
                      ctx->ebx,
                      ctx->ecx,
                      ctx->edx,
                      ctx->esi,
                      ctx->edi,
                      ctx->eip,
                      ctx->cs);
        panic("EXCPT_BOUNDS", msg_buf);
        break;

    case 6:
        // Invalid Opcode
        mini_snprintf(msg_buf, PANIC_MSG_BUF_MAX,
                      "INT 6: Invalid Opcode exception \n\n"
                      "Context:\n"
                      " EAX: %u\n"
                      " EBX: %u\n"
                      " ECX: %u\n"
                      " EDX: %u\n"
                      " ESI: 0x%x\n"
                      " EDI: 0x%x\n"
                      " EFLAGS: 0x%x\n"
                      " EIP: 0x%x\n"
                      " CS: 0x%x\n",
                      ctx->eax,
                      ctx->ebx,
                      ctx->ecx,
                      ctx->edx,
                      ctx->esi,
                      ctx->edi,
                      ctx->eip,
                      ctx->cs);
        panic("EXCPT_INVOPC", msg_buf);
        break;
        break;

    case 7:
        // Coprocessor Not Available
        mini_snprintf(msg_buf, PANIC_MSG_BUF_MAX,
                      "INT 7: Coprocessor Not Available exception \n\n"
                      "Context:\n"
                      " EAX: %u\n"
                      " EBX: %u\n"
                      " ECX: %u\n"
                      " EDX: %u\n"
                      " ESI: 0x%x\n"
                      " EDI: 0x%x\n"
                      " EFLAGS: 0x%x\n"
                      " EIP: 0x%x\n"
                      " CS: 0x%x\n",
                      ctx->eax,
                      ctx->ebx,
                      ctx->ecx,
                      ctx->edx,
                      ctx->esi,
                      ctx->edi,
                      ctx->eip,
                      ctx->cs);
        panic("EXCPT_COPRNOTAVAIL", msg_buf);
        break;

    case 8:
        // Double Fault
        mini_snprintf(msg_buf, PANIC_MSG_BUF_MAX,
                      "INT 8: Double Fault exception \n\n"
                      "Context:\n"
                      " EAX: %u\n"
                      " EBX: %u\n"
                      " ECX: %u\n"
                      " EDX: %u\n"
                      " ESI: 0x%x\n"
                      " EDI: 0x%x\n"
                      " EFLAGS: 0x%x\n"
                      " EIP: 0x%x\n"
                      " CS: 0x%x\n",
                      ctx->eax,
                      ctx->ebx,
                      ctx->ecx,
                      ctx->edx,
                      ctx->esi,
                      ctx->edi,
                      ctx->eip,
                      ctx->cs);
        panic("EXCPT_DOUBFLT", msg_buf);
        break;

    case 9:
        // Coprocessor Segment Overrun
        mini_snprintf(msg_buf, PANIC_MSG_BUF_MAX,
                      "INT 9: Coprocessor Segment Overrun exception\n\n"
                      "Context:\n"
                      " EAX: %u\n"
                      " EBX: %u\n"
                      " ECX: %u\n"
                      " EDX: %u\n"
                      " ESI: 0x%x\n"
                      " EDI: 0x%x\n"
                      " EFLAGS: 0x%x\n"
                      " EIP: 0x%x\n"
                      " CS: 0x%x\n",
                      ctx->eax,
                      ctx->ebx,
                      ctx->ecx,
                      ctx->edx,
                      ctx->esi,
                      ctx->edi,
                      ctx->eip,
                      ctx->cs);
        panic("EXCPT_COPRSEGOVERRUN", msg_buf);
        break;

    case 10:
        // Invalid TSS
        mini_snprintf(msg_buf, PANIC_MSG_BUF_MAX,
                      "INT 10: Invalid TSS excpetion \n"
                      " Error code: 0x%x\n\n"
                      "Context:\n"
                      " EAX: %u\n"
                      " EBX: %u\n"
                      " ECX: %u\n"
                      " EDX: %u\n"
                      " ESI: 0x%x\n"
                      " EDI: 0x%x\n"
                      " EFLAGS: 0x%x\n"
                      " EIP: 0x%x\n"
                      " CS: 0x%x\n",
                      ctx->errco,
                      ctx->eax,
                      ctx->ebx,
                      ctx->ecx,
                      ctx->edx,
                      ctx->esi,
                      ctx->edi,
                      ctx->eip,
                      ctx->cs);
        panic("EXCPT_INVTSS", msg_buf);
        break;

    case 11:
        // Segment Not Present
        mini_snprintf(msg_buf, PANIC_MSG_BUF_MAX,
                      "INT 11: Segment Not Present excpetion \n"
                      " Error code: 0x%x\n\n"
                      "Context:\n"
                      " EAX: %u\n"
                      " EBX: %u\n"
                      " ECX: %u\n"
                      " EDX: %u\n"
                      " ESI: 0x%x\n"
                      " EDI: 0x%x\n"
                      " EFLAGS: 0x%x\n"
                      " EIP: 0x%x\n"
                      " CS: 0x%x\n",
                      ctx->errco,
                      ctx->eax,
                      ctx->ebx,
                      ctx->ecx,
                      ctx->edx,
                      ctx->esi,
                      ctx->edi,
                      ctx->eip,
                      ctx->cs);
        panic("EXCPT_SEGNOTPRESENT", msg_buf);
        break;

    case 12:
        // Stack
        mini_snprintf(msg_buf, PANIC_MSG_BUF_MAX,
                      "INT 12: Stack excpetion \n"
                      " Error code: 0x%x\n\n"
                      "Context:\n"
                      " EAX: %u\n"
                      " EBX: %u\n"
                      " ECX: %u\n"
                      " EDX: %u\n"
                      " ESI: 0x%x\n"
                      " EDI: 0x%x\n"
                      " EFLAGS: 0x%x\n"
                      " EIP: 0x%x\n"
                      " CS: 0x%x\n",
                      ctx->errco,
                      ctx->eax,
                      ctx->ebx,
                      ctx->ecx,
                      ctx->edx,
                      ctx->esi,
                      ctx->edi,
                      ctx->eip,
                      ctx->cs);
        panic("EXCPT_STACK", msg_buf);
        break;

    case 13:
        // General Protection
        mini_snprintf(msg_buf, PANIC_MSG_BUF_MAX,
                      "INT 13: General Protection excpetion \n"
                      "Error code: 0x%x\n\n"
                      "Context:\n"
                      " EAX: %u\n"
                      " EBX: %u\n"
                      " ECX: %u\n"
                      " EDX: %u\n"
                      " ESI: 0x%x\n"
                      " EDI: 0x%x\n"
                      " EFLAGS: 0x%x\n"
                      " EIP: 0x%x\n"
                      " CS: 0x%x\n",
                      ctx->errco,
                      ctx->eax,
                      ctx->ebx,
                      ctx->ecx,
                      ctx->edx,
                      ctx->esi,
                      ctx->edi,
                      ctx->eflags,
                      ctx->eip,
                      ctx->cs);
        panic("EXCPT_GENPROTECT", msg_buf);
        break;

    case 14:
        // General Protection
        mini_snprintf(msg_buf, PANIC_MSG_BUF_MAX,
                      "INT 14: Page Fault excpetion \n"
                      " Error code: 0x%x\n"
                      " CR2: 0x%x\n\n"
                      "Context:\n"
                      " EAX: %u\n"
                      " EBX: %u\n"
                      " ECX: %u\n"
                      " EDX: %u\n"
                      " ESI: 0x%x\n"
                      " EDI: 0x%x\n"
                      " EFLAGS: 0x%x\n"
                      " EIP: 0x%x\n"
                      " CS: 0x%x\n",
                      ctx->errco,
                      get_cr2_value(),
                      ctx->eax,
                      ctx->ebx,
                      ctx->ecx,
                      ctx->edx,
                      ctx->esi,
                      ctx->edi,
                      ctx->eflags,
                      ctx->eip,
                      ctx->cs);
        panic("EXCPT_PAGEFAULT", msg_buf);
        break;

    case 16:
        // Coprocessor Error
        mini_snprintf(msg_buf, PANIC_MSG_BUF_MAX,
                      "INT 16: Coprocessor Error exception\n\n"
                      "Context:\n"
                      " EAX: %u\n"
                      " EBX: %u\n"
                      " ECX: %u\n"
                      " EDX: %u\n"
                      " ESI: 0x%x\n"
                      " EDI: 0x%x\n"
                      " EFLAGS: 0x%x\n"
                      " EIP: 0x%x\n"
                      " CS: 0x%x\n",
                      ctx->eax,
                      ctx->ebx,
                      ctx->ecx,
                      ctx->edx,
                      ctx->esi,
                      ctx->edi,
                      ctx->eip,
                      ctx->cs);
        panic("EXCPT_COPRERROR", msg_buf);
        break;
    }

    panic("EXCEPTION", "");
}

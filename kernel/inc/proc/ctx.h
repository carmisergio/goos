#ifndef _PROC_CTX_H
#define _PROC_CTX_H 1

#if !defined(_AS)
// C definitions

#include <stdint.h>

// Process context
typedef struct
{
    // Registers
    uint32_t edi;
    uint32_t esi;
    uint32_t edx;
    uint32_t ecx;
    uint32_t ebx;
    uint32_t eax;

    // Data segment
    uint32_t ds;

    // Return context
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
    uint32_t ebp;

} cpu_ctx_t;

#else
// Assembly offsets
#define PROC_CTX_EDI 0x0
#define PROC_CTX_ESI 0x4
#define PROC_CTX_EDX 0x8
#define PROC_CTX_ECX 0xC
#define PROC_CTX_EBX 0x10
#define PROC_CTX_EAX 0x14
#define PROC_CTX_DS 0x18
#define PROC_CTX_EIP 0x1C
#define PROC_CTX_CS 0x20
#define PROC_CTX_EFLAGS 0x24
#define PROC_CTX_ESP 0x28
#define PROC_CTX_SS 0x2C
#define PROC_CTX_EBP 0x30

#endif

#endif
//
// GOOS system call stubs
//

#include <stdint.h>

#include "string.h"

#define SYSCALL_INT 0x30

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

// Internal function prototyes
int32_t syscall_1(uint32_t syscall_n, uint32_t p1);
int32_t syscall_2(uint32_t syscall_n, uint32_t p1, uint32_t p2);
int32_t syscall_2_2(uint32_t syscall_n, uint32_t p1, uint32_t p2, uint32_t *o2);

void console_write(char *str, uint32_t n)
{
    syscall_2(SYSCALL_CONSOLE_WRITE, (uint32_t)str, n);
}

int32_t console_readline(char *str, uint32_t n)
{
    return syscall_2(SYSCALL_CONSOLE_READLINE, (uint32_t)str, n);
}

int32_t exit(int32_t status)
{
    return syscall_1(SYSCALL_EXIT, (uint32_t)status);
}

int32_t exec(char *path, int32_t *status)
{
    uint32_t n = strlen(path);
    return syscall_2_2(SYSCALL_EXEC, (uint32_t)path, n, (uint32_t *)status);
}

/* Internal functions */

// Generic system call with 1 parameter and a return value
int32_t syscall_1(uint32_t syscall_n, uint32_t p1)
{
    int32_t res;

    // Execute system call
    __asm__ volatile(
        "int %1"
        : "=a"(res) : "n"(SYSCALL_INT), "a"(syscall_n), "b"(p1));

    return res;
}

int32_t syscall_2(uint32_t syscall_n, uint32_t p1, uint32_t p2)
{
    int32_t res;

    // Execute system call
    __asm__ volatile(
        "int %1"
        : "=a"(res) : "n"(SYSCALL_INT), "a"(syscall_n), "b"(p1), "c"(p2));

    return res;
}

// Sysetm call with two parameters and two outputs
int32_t syscall_2_2(uint32_t syscall_n, uint32_t p1, uint32_t p2, uint32_t *o2)
{
    int32_t res, o2_tmp;

    // Execute system call
    __asm__ volatile(
        "int %2"
        : "=a"(res), "=b"(o2_tmp) : "n"(SYSCALL_INT), "a"(syscall_n), "b"(p1), "c"(p2));

    *o2 = o2_tmp;

    return res;
}

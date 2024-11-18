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
} syscall_n_t;

// Internal function prototyes
int32_t syscall_0(uint32_t syscall_n);
int32_t syscall_1(uint32_t syscall_n, uint32_t p1);
int32_t syscall_2(uint32_t syscall_n, uint32_t p1, uint32_t p2);
int32_t syscall_2_2(uint32_t syscall_n, uint32_t p1, uint32_t p2, uint32_t *o2);

uint32_t _g_get_local_time()
{
    return (uint32_t)syscall_0(SYSCALL_GET_LOCAL_TIME);
}

void _g_delay_ms(uint32_t time)
{
    syscall_1(SYSCALL_DELAY_MS, time);
}

void _g_console_write(const char *str, uint32_t n)
{
    syscall_2(SYSCALL_CONSOLE_WRITE, (uint32_t)str, n);
}

int32_t _g_console_readline(char *str, uint32_t n)
{
    return syscall_2(SYSCALL_CONSOLE_READLINE, (uint32_t)str, n);
}

char console_getchar()
{
    return syscall_0(SYSCALL_CONSOLE_GETCHAR);
}

int32_t _g_exit(int32_t status)
{
    return syscall_1(SYSCALL_EXIT, (uint32_t)status);
}

int32_t _g_exec(const char *path, int32_t *status)
{
    uint32_t n = strlen(path);
    return syscall_2_2(SYSCALL_EXEC, (uint32_t)path, n, (uint32_t *)status);
}

int32_t _g_change_cwd(const char *path)
{
    uint32_t n = strlen(path);
    return syscall_2(SYSCALL_CHANGE_CWD, (uint32_t)path, n);
}

int32_t _g_get_cwd(char *buf)
{
    return syscall_1(SYSCALL_GET_CWD, (uint32_t)buf);
}

/* Internal functions */

// Generic system call with no parameters and a return value
int32_t syscall_0(uint32_t syscall_n)
{
    int32_t res;

    // Execute system call
    __asm__ volatile(
        "int %1"
        : "=a"(res) : "n"(SYSCALL_INT), "a"(syscall_n));

    return res;
}

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

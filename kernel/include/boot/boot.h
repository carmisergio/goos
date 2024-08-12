#ifndef _BOOT_BOOT_H
#define _BOOT_BOOT_H 1

#include <stdint.h>

extern uint32_t bootstrap_page_dir;

// Kernel load information
extern char *_kernel_start;
extern char *_kernel_end;

#endif
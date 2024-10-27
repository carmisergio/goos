#ifndef _BOOT_BOOT_H
#define _BOOT_BOOT_H 1

#include <stdint.h>

// Initial memory structures
extern uint32_t bootstrap_page_dir;
extern uint32_t kernel_stack_top;

// Kernel load information
extern uint8_t *_kernel_start;
extern uint8_t *_kernel_end;

#endif
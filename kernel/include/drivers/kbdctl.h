#ifndef _DRIVERS_KBDCTL
#define _DRIVERS_KBDCTL 1

#include <stdint.h>
#include <stdbool.h>

/*
 * Initialize the keyboard controller
 */
void kbdctl_init();

/*
 * Reset the CPU
 * This is accomplished by pulsing line 0 on the Keyboard Controller
 * output port
 */
void kbdctl_reset_cpu();

#endif
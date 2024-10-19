#ifndef _DRIVERS_KBDCTL
#define _DRIVERS_KBDCTL 1

#include <stdint.h>
#include <stdbool.h>

/*
 * Initialize the keyboard controller
 */
void kbdctl_init();

/*
 * Enable device IRQs
 */
void kbdctl_enable_irqs();

/*
 * Reset the CPU
 * This is accomplished by pulsing line 0 on the Keyboard Controller
 * output port
 */
void kbdctl_reset_cpu();

#endif
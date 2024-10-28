#ifndef _DRIVERS_PIC
#define _DRIVERS_PIC 1

#include <stdint.h>
#include <stdbool.h>

/*
 * Initialize the Programmable Interrupt Controller,
 * mapping the master PIC irqs to interrupt vectors [start_vec ... start_vec + 7]
 * and the slave PIC irqs to interrupt vectors [start_vec + 8 ... start_vec + 15]
 * #### Parameters
 *   - uint8_t start_vec: interrupt vector start index from which hadware IRQs
 *                        should be mapped to
 */
void pic_init(uint8_t start_vec);

/**
 * Send correct end of interrupt sequence for this IRQ
 * #### Parameters
 *   - uint8_t irq: irq that should be acknowledged
 */
void pic_send_eoi(uint8_t irq);

/*
 * Check if the interrupt was a spurious interrupt
 * If so, perform appropriate actions and return true.
 * Otherwise, return false
 * #### Parameters:
 *   - uint8_t irq: irq that was raised (can only be 7 -> master spurious irq,
 *                  or 15 -> slave spurious irq)
 * #### Returns:
 *   bool: wether or not this was a spurious IRQ
 */
bool pic_check_spurious(uint8_t irq);

#endif
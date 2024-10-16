#ifndef _DRIVERS_KBDCTL
#define _DRIVERS_KBDCTL 1

#include <stdint.h>
#include <stdbool.h>

/*
 * Initialize the keyboard controller
 */
void kbdctl_init();

/*
 * Read byte of data  from the keyboard controller
 * Blocks until data is available
 * #### Returns:
 *   uint8_t: data
 */
uint8_t kbdctl_read_data();

/*
 * Read byte of data  from the keyboard controller
 * Blocks until data is available or timeout expires
 * #### Parameters:
 *   - uint8_t *data: pointer to variable to put data
 *   - uint32_t timeout: timeout (ms)
 * #### Returns:
 *   bool: false if timed out, true otherwise
 */
bool kbdctl_read_data_timeout(uint8_t *data, uint32_t timeout);

/*
 * Check if there is any data in the keyboard controller's ouptut buffer
 * #### Returns:
 *   bool: true -> data available, false -> no data available
 */
bool kbdctl_outbuf_full();

/*
 * Perform interface test on port 1
 * #### Returns:
 *   bool: true -> test ok, false -> test failed
 */
bool kbdctl_test_port1();

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
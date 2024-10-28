#ifndef _DRIVERS_VGA_H
#define _DRIVERS_VGA_H 1

#include <stddef.h>
#include <stdint.h>

#include "console/console.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

typedef uint8_t vga_char;

/*
 * Initialize vga driver
 *
 * Params: void
 * Returns: void
 */
void vga_init(void);

/*
 * Initialize vga driver functions that require
 * memory management to be set up
 * Should be called only AFTER vmem_init();
 */
void vga_init_aftermem();

/*
 * Output character to VGA at specific position
 *
 * Params:
 *   - c: character to print (CP437 encoding)
 *    - row, col: position
 *    - fg, bg: color
 */
void vga_putchar(uint8_t c, uint16_t row, uint16_t col, console_color_t fg, console_color_t bg);

/*
 * Clear all characters from the screen, fill with backgorund
 * color specified
 *
 * Params:
 *   - bg: background color
 */
void vga_clearscr(console_color_t bg);

/*
 * Scroll all currently visible characters up by one line,
 * clearing the last line
 *
 * Params:
 *   - bg: background color for the new line
 */
void vga_scroll(console_color_t bg);

#endif
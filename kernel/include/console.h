#ifndef _CONSOLE_H
#define _CONSOLE_H 1

#include <stdint.h>
#include <stddef.h>

// Console colors
typedef enum
{
    CONSOLE_COLOR_BLACK,
    CONSOLE_COLOR_BLUE,
    CONSOLE_COLOR_GREEN,
    CONSOLE_COLOR_CYAN,
    CONSOLE_COLOR_RED,
    CONSOLE_COLOR_MAGENTA,
    CONSOLE_COLOR_BROWN,
    CONSOLE_COLOR_LIGHT_GREY,
    CONSOLE_COLOR_DARK_GREY,
    CONSOLE_COLOR_LIGHT_BLUE,
    CONSOLE_COLOR_LIGHT_GREEN,
    CONSOLESOLE_COLOR_LIGHT_CYAN,
    CONSOLE_COLOR_LIGHT_RED,
    CONSOLE_COLOR_LIGHT_MAGENTA,
    CONSOLE_COLOR_LIGHT_BROWN,
    CONSOLE_COLOR_WHITE,
} console_color_t;

/**
 * Initialize system console
 */
void console_init();

/**
 * Write to system console
 * Parses ANSI control sequences
 * #### Parameters:
 *   - s: string to print
 */
void console_write(char *s, size_t n);

/**
 * Clear console
 */
void console_clear();

/**
 * Set foreground color
 * #### Parameters
 *   - color: color to set
 */
void console_set_fgcol(console_color_t color);

/**
 * Set background color
 * #### Parameters
 *   - color: color to set
 */
void console_set_bgcol(console_color_t color);

/**
 * Set cursor position
 * #### Parameters
 *   - row: row
 *   - col: column
 */
void console_set_curspos(uint16_t row, uint16_t col);

#endif _CONSOLE_H
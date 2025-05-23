#ifndef _CONSOLE_CONSOLE_H
#define _CONSOLE_CONSOLE_H 1

#include <stdint.h>
#include <stddef.h>

// Console colors
typedef enum
{
    CONS_COL_BLACK,
    CONS_COL_RED,
    CONS_COL_GREEN,
    CONS_COL_YELLOW,
    CONS_COL_BLUE,
    CONS_COL_PURPLE,
    CONS_COL_CYAN,
    CONS_COL_WHITE,
    CONS_COL_HI_BLACK,
    CONS_COL_HI_RED,
    CONS_COL_HI_GREEN,
    CONS_COL_HI_YELLOW,
    CONS_COL_HI_BLUE,
    CONS_COL_HI_PURPLE,
    CONS_COL_HI_CYAN,
    CONS_COL_HI_WHITE,
} console_color_t;

/**
 * Initialize system console
 */
void console_init();

/**
 * Set up keyboard side of the console
 * Must be called AFTER the keyboard subsystem has been initialized
 */
void console_init_kbd();

/**
 * Reset console to its initial state
 */
void console_reset();

/**
 * Write to system console
 * Parses ANSI control sequences
 * #### Parameters:
 *   - s: string to print
 *   - n: length of string
 */
void console_write(char *s, size_t n);

/**
 * Read from system console
 * #### Parameters:
 *   - s: string to print
 *   - n: max number of bytes to read
 * #### Returns: number of bytes read
 */
int32_t console_readline(char *s, size_t n);

/**
 * Read single character from system console
 * Returns immediately when a character is read
 * Doesn't echo characters
 * #### Returns: character
 */
char _g_console_getchar();

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
 * Reset color to default
 */
void console_reset_color();

/**
 * Set cursor position
 * #### Parameters
 *   - row: row
 *   - col: column
 */
void console_set_curspos(uint16_t row, uint16_t col);

#endif
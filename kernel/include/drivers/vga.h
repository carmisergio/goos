#ifndef _DRIVERS_VGA_H
#define _DRIVERS_VGA_H 1

#include <stddef.h>
#include <stdint.h>

// VGA color constants
enum vga_color
{
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

#ifdef __cplusplus
extern "C"
{
#endif

    /*
     * Initialize vga driver
     *
     * Params: void
     * Returns: void
     */
    void vga_init(void);

    /*
     * Output character to VGA text console
     *
     * Params:
     *     com_port port: port to output to
     *     char c: character to output to serial
     * Returns: void
     */
    void vga_putchar(char c);

    /*
     * Print null terminated string to VGA text console
     *
     * Params:
     *     char *str: string to print
     * Returns: void
     */
    void vga_prtstr(const char *str);

    void vga_setcol(enum vga_color fg, enum vga_color bg);

    void vga_clearscreen();

#ifdef __cplusplus
}
#endif

#endif
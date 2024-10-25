#include <stdbool.h>

#include "drivers/vga.h"

#include "sys/io.h"
#include "mem/vmem.h"
#include "panic.h"

// VGA colors
typedef enum
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
} vga_color_t;

typedef uint16_t vga_entry_t;

// VGA constants
static void *VGA_BUF_ADDR = (void *)0xB8000;

// Console colors to VGA colors conversion table
static const vga_color_t color_conv_tbl[] = {
    [CONSOLE_COLOR_BLACK] = VGA_COLOR_BLACK,
    [CONSOLE_COLOR_BLUE] = VGA_COLOR_BLUE,
    [CONSOLE_COLOR_GREEN] = VGA_COLOR_GREEN,
    [CONSOLE_COLOR_CYAN] = VGA_COLOR_CYAN,
    [CONSOLE_COLOR_RED] = VGA_COLOR_RED,
    [CONSOLE_COLOR_MAGENTA] = VGA_COLOR_MAGENTA,
    [CONSOLE_COLOR_BROWN] = VGA_COLOR_BROWN,
    [CONSOLE_COLOR_LIGHT_GREY] = VGA_COLOR_LIGHT_GREY,
    [CONSOLE_COLOR_DARK_GREY] = VGA_COLOR_DARK_GREY,
    [CONSOLE_COLOR_LIGHT_BLUE] = VGA_COLOR_LIGHT_BLUE,
    [CONSOLE_COLOR_LIGHT_GREEN] = VGA_COLOR_LIGHT_GREEN,
    [CONSOLESOLE_COLOR_LIGHT_CYAN] = VGA_COLOR_LIGHT_CYAN,
    [CONSOLE_COLOR_LIGHT_RED] = VGA_COLOR_LIGHT_RED,
    [CONSOLE_COLOR_LIGHT_MAGENTA] = VGA_COLOR_LIGHT_MAGENTA,
    [CONSOLE_COLOR_LIGHT_BROWN] = VGA_COLOR_LIGHT_BROWN,
    [CONSOLE_COLOR_WHITE] = VGA_COLOR_WHITE,
};

// Pointer to screen buffer
vga_entry_t *vga_buffer;

static inline vga_entry_t vga_entry(char c, vga_color_t fg, vga_color_t bg);
static inline vga_color_t vga_color(console_color_t color);
void vga_disablecursor();
void vga_putentryat(vga_entry_t entry, uint16_t row, uint16_t col);
void _vga_scroll();
void _vga_newline();

/* Public functions */
void vga_init()
{
    vga_buffer = (uint16_t *)VGA_BUF_ADDR;

    // Clear screen
    vga_disablecursor();
}

void vga_init_aftermem()
{
    // Map VGA memory into KVAS
    vga_buffer = (uint16_t *)vmem_map_range_anyk(
        VGA_BUF_ADDR, VGA_WIDTH * VGA_HEIGHT * 2);
    // NOTE: 2 bytes per character

    // Check if mapping was succesful
    if (vga_buffer == NULL)
        panic("VGA_BUF_MAP_FAIL", "Unable to map VGA buffer into VAS");
}

void vga_putchar(char c, uint16_t row, uint16_t col, console_color_t fg, console_color_t bg)
{
    vga_entry_t entry = vga_entry(c, vga_color(fg), vga_color(bg));
    vga_putentryat(entry, row, col);
}

void vga_clearscr(console_color_t bg)
{

    uint16_t entry = vga_entry(' ', vga_color(bg), vga_color(bg));
    size_t index;

    for (size_t row = 0; row < VGA_HEIGHT; row++)
    {
        for (size_t col = 0; col < VGA_WIDTH; col++)
        {
            index = row * VGA_WIDTH + col;
            vga_buffer[index] = entry;
        }
    }
}

void vga_scroll(console_color_t bg)
{
    for (size_t row = 0; row < VGA_HEIGHT - 1; row++)
    {
        // Move line
        for (size_t i = row * VGA_WIDTH; i < (row + 1) * VGA_WIDTH; i++)
            vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }

    // Clear last line
    uint16_t entry = vga_entry(' ', vga_color(bg), vga_color(bg));
    for (size_t i = 24 * VGA_WIDTH; i < 25 * VGA_WIDTH; i++)
        vga_buffer[i] = entry;
}

/* Internal functions */
static inline vga_entry_t vga_entry(char c, vga_color_t fg, vga_color_t bg)
{

    // COLOR = | BG | FG |
    uint8_t col = fg | bg << 4;

    // | COLOR | CHARACTER |
    return (uint16_t)c | (uint16_t)col << 8;
}

static inline vga_color_t vga_color(console_color_t color)
{
    return color_conv_tbl[color];
}

void vga_disablecursor()
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void vga_putentryat(uint16_t entry, uint16_t row, uint16_t col)
{
    size_t index = row * VGA_WIDTH + col;
    vga_buffer[index] = entry;
}
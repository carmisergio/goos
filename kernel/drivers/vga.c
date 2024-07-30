#include "sys/io.h"
#include "drivers/vga.h"

// VGA constants
static const size_t VGA_CONSOLE_WIDTH = 80;
static const size_t VGA_CONSOLE_HEIGHT = 25;
uint16_t *vga_buffer;

// Current VGA driver state
size_t vga_row;
size_t vga_col;
uint8_t vga_color_cur;

static inline uint16_t _vga_entry(char c, uint8_t col);
static inline uint8_t _vga_entry_color(enum vga_color fg, enum vga_color bg);
void _vga_disablecursor();
void _vga_putentryat(uint16_t entry, size_t row, size_t col);
void _vga_scroll();
void _vga_newline();

/* Public functions */
void vga_init()
{
    // Initialize terminal buffer
    vga_buffer = (uint16_t *)0xB8000;

    // Set up state
    vga_row = 0;
    vga_col = 0;
    vga_color_cur = _vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);

    // Clear screen
    _vga_disablecursor();
    vga_clearscreen();
}

void vga_putchar(char c)
{
    // Handle \n character
    if (c == '\n')
    {
        _vga_newline();
        return;
    }

    _vga_putentryat(_vga_entry(c, vga_color_cur), vga_row, vga_col);

    // Wrap text to new line
    vga_col++;
    if (vga_col >= VGA_CONSOLE_WIDTH)
        _vga_newline();
}

void vga_prtstr(const char *str)
{
    while (*str)
    {
        vga_putchar(*str);
        str++;
    }
}

void vga_setcol(enum vga_color fg, enum vga_color bg)
{
    vga_color_cur = _vga_entry_color(fg, bg);
}

void vga_clearscreen()
{
    uint16_t entry = _vga_entry(' ', vga_color_cur);
    size_t index;

    for (size_t row = 0; row < VGA_CONSOLE_HEIGHT; row++)
    {
        for (size_t col = 0; col < VGA_CONSOLE_WIDTH; col++)
        {
            index = row * VGA_CONSOLE_WIDTH + col;
            vga_buffer[index] = entry;
        }
    }

    // Reset cursor
    vga_col = 0;
    vga_row = 0;
}

/* Internal functions */
static inline uint16_t _vga_entry(char c, uint8_t col)
{
    // | COLOR | CHARACTER |
    return (uint16_t)c | (uint16_t)col << 8;
}

static inline uint8_t _vga_entry_color(enum vga_color fg, enum vga_color bg)
{
    return fg | bg << 4;
}

void _vga_disablecursor()
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void _vga_putentryat(uint16_t entry, size_t row, size_t col)
{
    size_t index = row * VGA_CONSOLE_WIDTH + col;
    vga_buffer[index] = entry;
}

void _vga_scroll()
{
    for (size_t row = 0; row < VGA_CONSOLE_HEIGHT - 1; row++)
    {
        // Move line
        for (size_t i = row * VGA_CONSOLE_WIDTH; i < (row + 1) * VGA_CONSOLE_WIDTH; i++)
            vga_buffer[i] = vga_buffer[i + VGA_CONSOLE_WIDTH];
    }

    // Clear last line
    uint16_t entry = _vga_entry(' ', vga_color_cur);
    for (size_t i = 24 * VGA_CONSOLE_WIDTH; i < 25 * VGA_CONSOLE_WIDTH; i++)
        vga_buffer[i] = entry;

    vga_row--;
}

void _vga_newline()
{
    vga_row++;
    vga_col = 0;

    if (vga_row >= VGA_CONSOLE_HEIGHT)
        _vga_scroll();
}

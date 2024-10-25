#include "console.h"

#include <stdbool.h>

#include "drivers/vga.h"

#define DEFAULT_COLOR_FG CONSOLE_COLOR_LIGHT_GREEN
#define DEFAULT_COLOR_BG CONSOLE_COLOR_BLACK

// Global console state
typedef struct
{
    // Console information
    uint16_t width, height;

    // Colors
    console_color_t fg, bg;

    // Cursor position
    uint16_t cur_row, cur_col;
    bool newline_adj;
} console_state_t;

// Internal functions
static void do_newline();
static void do_scroll();

// Global objects
static console_state_t cstate;

void console_init()
{
    // Set up console state
    cstate.fg = DEFAULT_COLOR_FG;
    cstate.bg = DEFAULT_COLOR_BG;
    cstate.cur_row = 0;
    cstate.cur_col = 0;
    cstate.width = VGA_WIDTH;
    cstate.height = VGA_HEIGHT;
    cstate.newline_adj = false;

    // Initialize VGA driver
    vga_init();

    // Clear screen
    console_clear();
}

void console_write(char *s, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {

        // Handle newlines
        if (s[i] == '\n')
        {
            if (cstate.newline_adj)
                cstate.newline_adj = false;
            else
                do_newline();
            return;
        }

        vga_putchar(s[i], cstate.cur_row, cstate.cur_col, cstate.fg, cstate.bg);

        // Update cursor
        cstate.cur_col++;
        if (cstate.cur_col > cstate.width)
        {
            do_newline();
            cstate.newline_adj = true;
        }
    }
}

void console_clear()
{
    vga_clearscr(cstate.bg);
}

void console_set_fgcol(console_color_t color)
{
    cstate.fg = color;
}

void console_set_bgcol(console_color_t color)
{
    cstate.bg = color;
}

void console_set_curspos(uint16_t row, uint16_t col)
{
    cstate.cur_row = row;
    cstate.cur_col = col;
}

/* Internal functions */

static void do_newline()
{
    // Adjust cursor position
    cstate.cur_row++;
    cstate.cur_col = 0;

    // Check if its time to scroll
    if (cstate.cur_row >= cstate.height)
        do_scroll();
}

// Perform scroll
static void do_scroll()
{
    // Scroll text
    vga_scroll(cstate.bg);

    // Adjust cursor position
    cstate.cur_row--;
}
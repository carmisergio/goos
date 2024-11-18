#include "console/console.h"

#include <stdbool.h>

#include "drivers/vga.h"
#include "log.h"
#include "kbd/kbd.h"
#include "cpu.h"
#include "console/ascii.h"

// #define DEFAULT_COLOR_FG CONS_COL_HI_GREEN
#define DEFAULT_COLOR_FG CONS_COL_HI_GREEN
#define DEFAULT_COLOR_BG CONS_COL_BLACK

#define CHAR_BELL 0x07
#define CHAR_BACKSPACE 0x08
#define CHAR_HTAB 0x09
#define CHAR_NEWLINE 0x0A
#define CHAR_VTAB 0x0B
#define CHAR_FORMFEED 0x0C
#define CHAR_CRETURN 0x0D
#define CHAR_ESC 0x1B
#define CHAR_DEL 0x7F

// CP437 characters
#define CP437_BLOCK 0xDB

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

typedef enum
{
    NOFIRST,
    RESET,
    FOREGROUND,
    BACKGROUND,
} ansi_color_parse_state_t;

// Internal functions
static void putchar(char c);
static void do_newline();
static void do_scroll();
static kbd_event_t wait_key();
static void kbd_event_receiver(kbd_event_t e);
static void do_backspace();

// ASCII conversion tables
extern uint8_t normal_table[];

// Global objects
static console_state_t cstate;
static kbd_event_t kbd_event_buf;
static bool kbd_event_received;

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

void console_init_kbd()
{
    // Register keyboard event handler
    kbd_register_kbd_event_recv(kbd_event_receiver);
}

void console_reset()
{
    // Set up console state
    cstate.fg = DEFAULT_COLOR_FG;
    cstate.bg = DEFAULT_COLOR_BG;
    cstate.cur_row = 0;
    cstate.cur_col = 0;
    cstate.width = VGA_WIDTH;
    cstate.height = VGA_HEIGHT;
    cstate.newline_adj = false;
}

// TODO: refactor
void console_write(char *s, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        // Process ANSI
        if (s[i] == CHAR_ESC)
        {
            if (++i >= n)
                return;

            // Parse CSI
            if (s[i] == '[')
            {
                if (++i >= n)
                    return;

                if (s[i] == 'H')
                {
                    // Home cursor
                    console_set_curspos(0, 0);
                    continue;
                }
                else if (s[i] == '2')
                {
                    if (++i >= n)
                        return;

                    if (s[i] == 'J')
                    {
                        // Clear screen
                        console_clear();
                        continue;
                    }
                }
                else if (s[i] == '3' || s[i] == '4' || s[i] == '9' || s[i] == '1' || s[i] == '0')
                {
                    ansi_color_parse_state_t st = NOFIRST;
                    bool hi = false, seq_done = false;
                    console_color_t col;
                    while (i < n)
                    {

                        // End of sequence
                        if (s[i] == 'm')
                        {
                            if (++i >= n)
                                return;
                            break;
                        }

                        if (st == NOFIRST)
                        {
                            // Read first character
                            switch (s[i])
                            {
                            case '3':
                                st = FOREGROUND;
                                hi = false;
                                break;
                            case '4':
                                st = BACKGROUND;
                                hi = false;
                                break;
                            case '9':
                                st = FOREGROUND;
                                hi = true;
                                break;
                            case '1':
                                st = BACKGROUND;
                                hi = true;
                                if (++i >= n)
                                    return;
                                break;
                            case '0':
                                st = RESET;
                                seq_done = true;
                                break;
                            }
                        }
                        else
                        {
                            switch (s[i])
                            {
                            case '0':
                                col = hi ? CONS_COL_HI_BLACK : CONS_COL_BLACK;
                                seq_done = true;
                                break;
                            case '1':
                                col = hi ? CONS_COL_HI_RED : CONS_COL_RED;
                                seq_done = true;
                                break;
                            case '2':
                                col = hi ? CONS_COL_HI_GREEN : CONS_COL_GREEN;
                                seq_done = true;
                                break;
                            case '3':
                                col = hi ? CONS_COL_HI_YELLOW : CONS_COL_YELLOW;
                                seq_done = true;
                                break;
                            case '4':
                                col = hi ? CONS_COL_HI_BLUE : CONS_COL_BLUE;
                                seq_done = true;
                                break;
                            case '5':
                                col = hi ? CONS_COL_HI_PURPLE : CONS_COL_PURPLE;
                                seq_done = true;
                                break;
                            case '6':
                                col = hi ? CONS_COL_HI_CYAN : CONS_COL_CYAN;
                                seq_done = true;
                                break;
                            case '7':
                                col = hi ? CONS_COL_HI_WHITE : CONS_COL_WHITE;
                                seq_done = true;
                                break;
                            }
                        }

                        if (seq_done)
                        {
                            if (st == FOREGROUND)
                                console_set_fgcol(col);
                            else if (st == BACKGROUND)
                                console_set_bgcol(col);
                            else if (st == RESET)
                                console_reset_color();

                            // Reset parser state
                            st = NOFIRST;
                            hi = false;
                            seq_done = false;
                        }

                        i++;
                    }
                }
            }
        }

        // Non-ANSI character
        putchar(s[i]);
    }
}

int32_t console_readline(char *s, size_t n)
{
    kbd_event_t kbd_event;
    uint32_t read_n = 0;

    while (true)
    {
        // Draw cursor
        vga_putchar(CP437_BLOCK, cstate.cur_row, cstate.cur_col, cstate.fg, cstate.bg);

        kbd_event = wait_key();

        // Clear cursor
        vga_putchar(' ', cstate.cur_row, cstate.cur_col, cstate.fg, cstate.bg);

        // Special keys
        switch (kbd_event.keysym)
        {
        case KS_ENTER:
            // End of input
            putchar('\n');
            return read_n;
        case KS_BACKSPACE:
            if (read_n > 0)
            {
                do_backspace();
                read_n--;
            }
            continue;
        }

        // CTRL keys
        if (kbd_event.mod.ctrl)
        {
            switch (kbd_event.keysym)
            {
            case KS_u:
                // Delete whole line
                for (size_t i = 0; i < read_n; i++)
                    do_backspace();
                read_n = 0;
                break;
            }
        }

        // Get ascii representation of key
        uint8_t ascii = kbd_event_to_ascii(kbd_event);

        // Ignore non ascii characters
        if (ascii == 0)
            continue;

        // Ignore characters after end of read buffer
        if (read_n >= n)
            continue;

        // Echo character
        putchar(ascii);

        // Save character in read buffer
        s[read_n] = ascii;

        // Next char
        read_n++;
    }
}

char console_getchar()
{
    char ascii;

    // Wait until a valid character
    do
    {
        kbd_event_t kbd_event = wait_key();
        ascii = kbd_event_to_ascii(kbd_event);
    } while (!ascii);

    return ascii;
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

void console_reset_color()
{
    cstate.fg = DEFAULT_COLOR_FG;
    cstate.bg = DEFAULT_COLOR_BG;
}

void console_set_curspos(uint16_t row, uint16_t col)
{
    cstate.cur_row = row;
    cstate.cur_col = col;
}

/* Internal functions */

static void putchar(char c)
{
    // Handle special characters
    switch (c)
    {
    case CHAR_BACKSPACE:
        if (cstate.cur_col != 0)
        {
            cstate.cur_col--;
        }
        else
        {
            cstate.cur_col = cstate.width - 1;
            if (cstate.cur_row != 0)
                cstate.cur_row--;
        }
        return;
    case CHAR_NEWLINE:
        if (!cstate.newline_adj)
            do_newline();
        return;
    case CHAR_CRETURN:
        cstate.cur_col = 0;
        return;
    case CHAR_DEL:
        vga_putchar(' ', cstate.cur_row, cstate.cur_col, cstate.fg, cstate.bg);
        return;
    }

    cstate.newline_adj = false;

    vga_putchar(c, cstate.cur_row, cstate.cur_col, cstate.fg, cstate.bg);

    // Update cursor
    cstate.cur_col++;
    if (cstate.cur_col > cstate.width)
    {
        do_newline();
        cstate.newline_adj = true;
    }
}

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

// Delete last character
static void do_backspace()
{
    if (cstate.cur_col > 0)
    {
        cstate.cur_col--;
    }
    else if (cstate.cur_row > 0)
    {
        cstate.cur_col = cstate.width - 1;
        cstate.cur_row--;
    }

    // Delete character
    vga_putchar(' ', cstate.cur_row, cstate.cur_col, cstate.fg, cstate.bg);
}

// Wait for a keyboard event to come in from the keyboard subsystem
static kbd_event_t wait_key()
{
    // Wait for key to be pressed
    kbd_event_received = false;
    while (!kbd_event_received)
        pause();

    return kbd_event_buf;
}

// Handles keyboard events from the keyboard subsystem
static void kbd_event_receiver(kbd_event_t e)
{
    kbd_event_buf = e;
    kbd_event_received = true;
}

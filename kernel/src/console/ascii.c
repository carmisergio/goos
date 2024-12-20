#include "console/ascii.h"

#include "kbd/codes.h"

static uint8_t normal_table[256] = {
    // Alphanumeric
    [KS_a] = 'a',
    [KS_b] = 'b',
    [KS_c] = 'c',
    [KS_d] = 'd',
    [KS_e] = 'e',
    [KS_f] = 'f',
    [KS_g] = 'g',
    [KS_h] = 'h',
    [KS_i] = 'i',
    [KS_j] = 'j',
    [KS_k] = 'k',
    [KS_l] = 'l',
    [KS_m] = 'm',
    [KS_n] = 'n',
    [KS_o] = 'o',
    [KS_p] = 'p',
    [KS_q] = 'q',
    [KS_r] = 'r',
    [KS_s] = 's',
    [KS_t] = 't',
    [KS_u] = 'u',
    [KS_v] = 'v',
    [KS_w] = 'w',
    [KS_x] = 'x',
    [KS_y] = 'y',
    [KS_z] = 'z',
    [KS_A] = 'A',
    [KS_B] = 'B',
    [KS_C] = 'C',
    [KS_D] = 'D',
    [KS_E] = 'E',
    [KS_F] = 'F',
    [KS_G] = 'G',
    [KS_H] = 'H',
    [KS_I] = 'I',
    [KS_J] = 'J',
    [KS_K] = 'K',
    [KS_L] = 'L',
    [KS_M] = 'M',
    [KS_N] = 'N',
    [KS_O] = 'O',
    [KS_P] = 'P',
    [KS_Q] = 'Q',
    [KS_R] = 'R',
    [KS_S] = 'S',
    [KS_T] = 'T',
    [KS_U] = 'U',
    [KS_V] = 'V',
    [KS_W] = 'W',
    [KS_X] = 'X',
    [KS_Y] = 'Y',
    [KS_Z] = 'Z',
    [KS_0] = '0',
    [KS_1] = '1',
    [KS_2] = '2',
    [KS_3] = '3',
    [KS_4] = '4',
    [KS_5] = '5',
    [KS_6] = '6',
    [KS_7] = '7',
    [KS_8] = '8',
    [KS_9] = '9',

    // Special characters
    [KS_SPACE] = ' ',
    [KS_EXCL] = '!',
    [KS_DBLQT] = '"',
    [KS_HASHTAG] = '#',
    [KS_DOLLAR] = '$',
    [KS_PERCENT] = '%',
    [KS_AND] = '&',
    [KS_SNGLQT] = '\'',
    [KS_RBRACOP] = '(',
    [KS_RBRACCL] = ')',
    [KS_ASTRSK] = '*',
    [KS_PLUS] = '+',
    [KS_COMMA] = ',',
    [KS_DASH] = '-',
    [KS_PERIOD] = '.',
    [KS_SLASH] = '/',
    [KS_COL] = ':',
    [KS_SMCOL] = ';',
    [KS_LESSTHAN] = '<',
    [KS_EQUALS] = '=',
    [KS_GRTRTHAN] = '>',
    [KS_QUESTION] = '?',
    [KS_AT] = '@',
    [KS_SBRACOP] = '[',
    [KS_BKSLASH] = '\\',
    [KS_SBRACCL] = ']',
    [KS_CARET] = '^',
    [KS_UNDERSCORE] = '_',
    [KS_BKTICK] = '`',
    [KS_CBRACOP] = '{',
    [KS_PIPE] = '|',
    [KS_CBRACCL] = '}',
    [KS_TILDE] = '~',

    // Control keys
    [KS_BACKSPACE] = '\b',
    [KS_ENTER] = '\n',
    // [KS_TAB] = '\t',
};

uint8_t kbd_event_to_ascii(kbd_event_t e)
{
    // No modifiers
    if (!e.mod.ctrl && !e.mod.alt && !e.mod.super)
        return normal_table[e.keysym];

    return '\0';
}
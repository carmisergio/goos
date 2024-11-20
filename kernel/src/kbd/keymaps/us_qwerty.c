#include "kbd/keymap.h"
#include "kbd/codes.h"

#define ENABLE 1

#ifdef ENABLE

// Normal map
kbd_keysym_t normal_map[N_KEYCODES] = {
    [0x01] = KS_ESC,
    [0x02] = KS_F1,
    [0x03] = KS_F2,
    [0x04] = KS_F3,
    [0x05] = KS_F4,
    [0x06] = KS_F5,
    [0x07] = KS_F6,
    [0x08] = KS_F7,
    [0x09] = KS_F8,
    [0x0A] = KS_F9,
    [0x0B] = KS_F10,
    [0x0C] = KS_F11,
    [0x0D] = KS_F12,
    [0x0E] = KS_BKTICK,
    [0x0F] = KS_1,
    [0x10] = KS_2,
    [0x11] = KS_3,
    [0x12] = KS_4,
    [0x13] = KS_5,
    [0x14] = KS_6,
    [0x15] = KS_7,
    [0x16] = KS_8,
    [0x17] = KS_9,
    [0x18] = KS_0,
    [0x19] = KS_DASH,
    [0x1A] = KS_EQUALS,
    [0x1B] = KS_BACKSPACE,
    [0x1C] = KS_TAB,
    [0x1D] = KS_q,
    [0x1E] = KS_w,
    [0x1F] = KS_e,
    [0x20] = KS_r,
    [0x21] = KS_t,
    [0x22] = KS_y,
    [0x23] = KS_u,
    [0x24] = KS_i,
    [0x25] = KS_o,
    [0x26] = KS_p,
    [0x27] = KS_SBRACOP,
    [0x28] = KS_SBRACCL,
    [0x29] = KS_BKSLASH,
    [0x2B] = KS_a,
    [0x2C] = KS_s,
    [0x2D] = KS_d,
    [0x2E] = KS_f,
    [0x2F] = KS_g,
    [0x30] = KS_h,
    [0x31] = KS_j,
    [0x32] = KS_k,
    [0x33] = KS_l,
    [0x34] = KS_SMCOL,
    [0x35] = KS_SNGLQT,
    [0x36] = KS_ENTER,
    [0x38] = KS_LESSTHAN,
    [0x39] = KS_z,
    [0x3A] = KS_x,
    [0x3B] = KS_c,
    [0x3C] = KS_v,
    [0x3D] = KS_b,
    [0x3E] = KS_n,
    [0x3F] = KS_m,
    [0x40] = KS_COMMA,
    [0x41] = KS_PERIOD,
    [0x42] = KS_SLASH,
    [0x45] = KS_SUPER,
    [0x47] = KS_SPACE,
    [0x49] = KS_SUPER,
    [0x4A] = KS_MENU,
    [0x61] = KS_SLASH,
    [0x62] = KS_ASTRSK,
    [0x63] = KS_DASH,
    [0x64] = KS_HOME,
    [0x65] = KS_ARROWUP,
    [0x66] = KS_PGUP,
    [0x67] = KS_PLUS,
    [0x68] = KS_ARROWLEFT,
    [0x6A] = KS_ARROWRIGHT,
    [0x6B] = KS_END,
    [0x6C] = KS_ARROWDOWN,
    [0x6D] = KS_PGDOWN,
    [0x6E] = KS_ENTER,
    [0x6F] = KS_INSERT,
    [0x70] = KS_DEL,
    [0x83] = KS_INSERT,
    [0x84] = KS_HOME,
    [0x85] = KS_PGUP,
    [0x86] = KS_DEL,
    [0x87] = KS_END,
    [0x88] = KS_PGDOWN,
    [0x89] = KS_ARROWUP,
    [0x8A] = KS_ARROWLEFT,
    [0x8B] = KS_ARROWDOWN,
    [0x8C] = KS_ARROWRIGHT,
};

kbd_keysym_t numlock_map[N_KEYCODES] = {
    [0x64] = KS_7,
    [0x65] = KS_8,
    [0x66] = KS_9,
    [0x68] = KS_4,
    [0x69] = KS_5,
    [0x6A] = KS_6,
    [0x6B] = KS_1,
    [0x6C] = KS_2,
    [0x6D] = KS_3,
    [0x6F] = KS_0,
    [0x70] = KS_PERIOD,
};

kbd_keysym_t shift_map[N_KEYCODES] = {
    [0x0E] = KS_TILDE,
    [0x0F] = KS_EXCL,
    [0x10] = KS_AT,
    [0x11] = KS_HASHTAG,
    [0x12] = KS_DOLLAR,
    [0x13] = KS_PERCENT,
    [0x14] = KS_CARET,
    [0x15] = KS_AND,
    [0x16] = KS_ASTRSK,
    [0x17] = KS_RBRACOP,
    [0x18] = KS_RBRACCL,
    [0x19] = KS_UNDERSCORE,
    [0x1A] = KS_PLUS,
    [0x1D] = KS_Q,
    [0x1E] = KS_W,
    [0x1F] = KS_E,
    [0x20] = KS_R,
    [0x21] = KS_T,
    [0x22] = KS_Y,
    [0x23] = KS_U,
    [0x24] = KS_I,
    [0x25] = KS_O,
    [0x26] = KS_P,
    [0x27] = KS_CBRACOP,
    [0x28] = KS_CBRACCL,
    [0x29] = KS_PIPE,
    [0x34] = KS_COL,
    [0x2B] = KS_A,
    [0x2C] = KS_S,
    [0x2D] = KS_D,
    [0x2E] = KS_F,
    [0x2F] = KS_G,
    [0x30] = KS_H,
    [0x31] = KS_J,
    [0x32] = KS_K,
    [0x33] = KS_L,
    [0x35] = KS_DBLQT,
    [0x38] = KS_GRTRTHAN,
    [0x39] = KS_Z,
    [0x3A] = KS_X,
    [0x3B] = KS_C,
    [0x3C] = KS_V,
    [0x3D] = KS_B,
    [0x3E] = KS_N,
    [0x3F] = KS_M,
    [0x40] = KS_LESSTHAN,
    [0x41] = KS_GRTRTHAN,
    [0x42] = KS_QUESTION,
};

kbd_keysym_t caps_map[N_KEYSYMS] = {
    [KS_a] = KS_A,
    [KS_b] = KS_B,
    [KS_c] = KS_C,
    [KS_d] = KS_D,
    [KS_e] = KS_E,
    [KS_f] = KS_F,
    [KS_g] = KS_G,
    [KS_h] = KS_H,
    [KS_i] = KS_I,
    [KS_j] = KS_J,
    [KS_k] = KS_K,
    [KS_l] = KS_L,
    [KS_m] = KS_M,
    [KS_n] = KS_N,
    [KS_o] = KS_O,
    [KS_p] = KS_P,
    [KS_q] = KS_Q,
    [KS_r] = KS_R,
    [KS_s] = KS_S,
    [KS_t] = KS_T,
    [KS_u] = KS_U,
    [KS_v] = KS_V,
    [KS_w] = KS_W,
    [KS_x] = KS_X,
    [KS_y] = KS_Y,
    [KS_z] = KS_Z,
    [KS_A] = KS_a,
    [KS_B] = KS_b,
    [KS_C] = KS_c,
    [KS_D] = KS_d,
    [KS_E] = KS_e,
    [KS_F] = KS_f,
    [KS_G] = KS_g,
    [KS_H] = KS_h,
    [KS_I] = KS_i,
    [KS_J] = KS_j,
    [KS_K] = KS_k,
    [KS_L] = KS_l,
    [KS_M] = KS_m,
    [KS_N] = KS_n,
    [KS_O] = KS_o,
    [KS_P] = KS_p,
    [KS_Q] = KS_q,
    [KS_R] = KS_r,
    [KS_S] = KS_s,
    [KS_T] = KS_t,
    [KS_U] = KS_u,
    [KS_V] = KS_v,
    [KS_W] = KS_w,
    [KS_X] = KS_x,
    [KS_Y] = KS_y,
    [KS_Z] = KS_z,
};

const kbd_keymap_t kbd_keymap_us_qwerty = {
    .patch_map = KEYMAP_UNUSED,
    .normal_map = normal_map,
    .numlock_map = numlock_map,
    .shift_map = shift_map,
    .caps_map = caps_map,
    .altgr_map = KEYMAP_UNUSED,
};

#endif
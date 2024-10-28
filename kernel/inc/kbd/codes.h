#ifndef _KBD_CODES_H
#define _KBD_CODES_H 1

#define N_KEYCODES 256
#define N_KEYSYMS 256

// Reserved keycodes
#define KC_NULL 0x00
#define KC_IGNR 0xFF

// Modifier keycodes
#define KC_CAPSLCK 0x2A
#define KC_LSHIFT 0x37
#define KC_RSHIFT 0x43
#define KC_LCTRL 0x44
#define KC_LSUPER 0x45
#define KC_LALT 0x46
#define KC_RALT 0x48
#define KC_RSUPER 0x49
#define KC_RCTRL 0x4A
#define KC_ALTGR 0x4C
#define KC_NUMLCK 0x60
#define KC_SCRLLCK 0x81

//////// Keysyms

typedef enum
{
    KS_NULL = 0x00,

    // Function keys
    KS_ESC,
    KS_F1,
    KS_F2,
    KS_F3,
    KS_F4,
    KS_F5,
    KS_F6,
    KS_F7,
    KS_F8,
    KS_F9,
    KS_F10,
    KS_F11,
    KS_F12,

    // Alphanumeric
    KS_a,
    KS_b,
    KS_c,
    KS_d,
    KS_e,
    KS_f,
    KS_g,
    KS_h,
    KS_i,
    KS_j,
    KS_k,
    KS_l,
    KS_m,
    KS_n,
    KS_o,
    KS_p,
    KS_q,
    KS_r,
    KS_s,
    KS_t,
    KS_u,
    KS_v,
    KS_w,
    KS_x,
    KS_y,
    KS_z,
    KS_A,
    KS_B,
    KS_C,
    KS_D,
    KS_E,
    KS_F,
    KS_G,
    KS_H,
    KS_I,
    KS_J,
    KS_K,
    KS_L,
    KS_M,
    KS_N,
    KS_O,
    KS_P,
    KS_Q,
    KS_R,
    KS_S,
    KS_T,
    KS_U,
    KS_V,
    KS_W,
    KS_X,
    KS_Y,
    KS_Z,
    KS_0,
    KS_1,
    KS_2,
    KS_3,
    KS_4,
    KS_5,
    KS_6,
    KS_7,
    KS_8,
    KS_9,

    // Special characters
    KS_SPACE,
    KS_EXCL,
    KS_DBLQT,
    KS_HASHTAG,
    KS_DOLLAR,
    KS_PERCENT,
    KS_AND,
    KS_SNGLQT,
    KS_RBRACOP,
    KS_RBRACCL,
    KS_ASTRSK,
    KS_PLUS,
    KS_COMMA,
    KS_DASH,
    KS_PERIOD,
    KS_SLASH,
    KS_COL,
    KS_SMCOL,
    KS_LESSTHAN,
    KS_EQUALS,
    KS_GRTRTHAN,
    KS_QUESTION,
    KS_AT,
    KS_SBRACOP,
    KS_BKSLASH,
    KS_SBRACCL,
    KS_CARET,
    KS_UNDERSCORE,
    KS_BKTICK,
    KS_CBRACOP,
    KS_PIPE,
    KS_CBRACCL,
    KS_TILDE,

    // Control keys
    KS_DEL,
    KS_BACKSPACE,
    KS_ENTER,
    KS_TAB,
    KS_MENU,
    KS_SUPER,
    KS_INSERT,
    KS_HOME,
    KS_END,
    KS_PGUP,
    KS_PGDOWN,
    KS_ARROWUP,
    KS_ARROWDOWN,
    KS_ARROWLEFT,
    KS_ARROWRIGHT,
} kbd_keysym_code_t;

#endif

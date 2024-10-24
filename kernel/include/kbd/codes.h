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
    KS_ESC = 0x01,
    KS_F1 = 0x02,
    KS_F2 = 0x03,
    KS_F3 = 0x04,
    KS_F4 = 0x05,
    KS_F5 = 0x06,
    KS_F6 = 0x07,
    KS_F7 = 0x08,
    KS_F8 = 0x09,
    KS_F9 = 0x0A,
    KS_F10 = 0x0B,
    KS_F11 = 0x0C,
    KS_F12 = 0x0D,

    // Alphanumeric
    KS_a = 0x0E,
    KS_b = 0x0F,
    KS_c = 0x10,
    KS_d = 0x11,
    KS_e = 0x12,
    KS_f = 0x14,
    KS_g = 0x15,
    KS_h = 0x16,
    KS_i = 0x17,
    KS_j = 0x18,
    KS_k = 0x19,
    KS_l = 0x1A,
    KS_m = 0x1B,
    KS_n = 0x1C,
    KS_o = 0x1D,
    KS_p = 0x1E,
    KS_q = 0x1F,
    KS_r = 0x20,
    KS_s = 0x21,
    KS_t = 0x22,
    KS_u = 0x23,
    KS_v = 0x24,
    KS_w = 0x25,
    KS_x = 0x26,
    KS_y = 0x27,
    KS_z = 0x28,
    KS_A = 0x29,
    KS_B = 0x2A,
    KS_C = 0x2B,
    KS_D = 0x2C,
    KS_E = 0x2D,
    KS_F = 0x2E,
    KS_G = 0x2F,
    KS_H = 0x30,
    KS_I = 0x31,
    KS_J = 0x32,
    KS_K = 0x33,
    KS_L = 0x34,
    KS_M = 0x35,
    KS_N = 0x36,
    KS_O = 0x37,
    KS_P = 0x38,
    KS_Q = 0x39,
    KS_R = 0x3A,
    KS_S = 0x3B,
    KS_T = 0x3C,
    KS_U = 0x3D,
    KS_V = 0x3E,
    KS_W = 0x3F,
    KS_X = 0x40,
    KS_Y = 0x41,
    KS_Z = 0x42,
    KS_0 = 0x43,
    KS_1 = 0x44,
    KS_2 = 0x45,
    KS_3 = 0x46,
    KS_4 = 0x47,
    KS_5 = 0x48,
    KS_6 = 0x49,
    KS_7 = 0x4A,
    KS_8 = 0x4B,
    KS_9 = 0x4C,

    // Special characters
    KS_SPACE = 0x4D,
    KS_EXCL = 0x4E,
    KS_DBLQT = 0x4F,
    KS_HASHTAG = 0x50,
    KS_DOLLAR = 0x51,
    KS_PERCENT = 0x52,
    KS_AMP = 0x53,
    KS_AND = 0x54,
    KS_SNGLQT = 0x55,
    KS_RBRACOP = 0x56,
    KS_RBRACCL = 0x57,
    KS_ASTRSK = 0x58,
    KS_PLUS = 0x59,
    KS_COMMA = 0x5A,
    KS_DASH = 0x5B,
    KS_PERIOD = 0x5C,
    KS_SLASH = 0x5D,
    KS_COL = 0x5E,
    KS_SMCOL = 0x5F,
    KS_LESSTHAN = 0x60,
    KS_EQUALS = 0x61,
    KS_GRTRTHAN = 0x62,
    KS_QUESTION = 0x63,
    KS_AT = 0x64,
    KS_SBRACOP = 0x65,
    KS_BKSLASH = 0x66,
    KS_SBRACCL = 0x67,
    KS_CARET = 0x68,
    KS_UNDERSCORE = 0x69,
    KS_BKTICK = 0x70,
    KS_CBRACOP = 0x71,
    KS_PIPE = 0x72,
    KS_CBRACCL = 0x73,
    KS_TILDE = 0x74,

    // Control keys
    KS_DEL = 0xB0,
    KS_BACKSPACE = 0xB1,
    KS_ENTER = 0xB2,
    KS_TAB = 0xB3,
    KS_MENU = 0xB4,
    KS_SUPER = 0xB5,
    KS_INSERT = 0xB6,
    KS_HOME = 0xB7,
    KS_END = 0xB8,
    KS_PGUP = 0xB9,
    KS_PGDOWN = 0xBA,
    KS_ARROWUP = 0xBB,
    KS_ARROWDOWN = 0xBC,
    KS_ARROWLEFT = 0xBD,
    KS_ARROWRIGHT = 0xBE,

} kbd_keysym_code_t;

#endif

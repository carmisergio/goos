#ifndef _KBD_KEYMAP_H
#define _KBD_KEYMAP_H 1

#include "kbd/kbd.h"

#define KEYMAP_UNUSED 0x00

/*
 * Definition of the keymap interface
 *
 * A keymap must provide at least a normal map.
 * All other maps are optional. Maps are marked as optional by setting their
 * pointer to KEYMAP_UNUSED.
 *
 * ### Order of processing
 * The followint order is followed:
 *  -[kc]> Patch Map -[kc]> (Normal Map + NumLock Map + Shift Map)
 *  -[ks]> Caps Map -[ks]> AltGr Map
 * Where the letters on the arrows identify the symbols which are the symbols
 * outputed from a table and used to index into the next
 *
 * ## Patch Map
 * Eg. kbd_keycode_t patch_map[256];
 * The patch map can be used to arbitrarily assign any incoming keycode
 * to any other. This is used for example to map the right alt key (KC_RALT)
 * to AltGr (KC_ALTGR). Patch values set as KC_NULL will be discarded.
 *
 * ## Normal Map
 * Eg. kbd_keysym_t normal_map[256];
 * Provides the foundational mapping from keycode to KeySym. It is always applied
 * to incoming keycodes.
 *
 * ## NumLock Map
 * Eg. kbd_keysym_t numlock_map[256];
 * Is only applied when NumLock is active. If the value set for a key is not
 * KS_NULL, it overrides the Normal Map
 *
 * ## Shift Map
 * Eg. kbd_keysym_t shift_map[256];
 * Similar to NumLock map, but overrides both the Normal Map and the NumLock
 * map
 *
 * ## Caps Map
 * Eg. kbd_keysym_t caps_map[256];
 * Active only when Caps Lock active It's indexed by the result of the
 * previous maps.
 *
 * ## Caps Map
 * Eg. kbd_keysym_t shift_map[256];
 * Like Caps Map, but active only when AltGr is active
 *
 * Note: in all maps, values not set are automatically set to KS_NULL
 *       by the compiler
 */

// Keymap interface
typedef struct
{
    kbd_keycode_t *patch_map;
    kbd_keysym_t *normal_map;
    kbd_keysym_t *numlock_map;
    kbd_keysym_t *shift_map;
    kbd_keysym_t *caps_map;
    kbd_keysym_t *altgr_map;
} kbd_keymap_t;

// Keymaps
extern const kbd_keymap_t kbd_keymap_us_qwerty; // United States QWERTY

#endif
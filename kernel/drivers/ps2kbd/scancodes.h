#ifndef _DRIVERS_PS2KBD_SCANCODES
#define _DRIVERS_PS2KBD_SCANCODES 1

#include <stdint.h>

#include "kbd/kbd.h"

// NULL poscode
#define IGNR KC_NULL

// Special Scancodes
#define SC_EXTENDED 0xE0 // Scancode is in the extended table
#define SC_BREAK 0xF0    // Key release

/*
 * This file contains the conversion tables between PS/2 scancodes
 * and internal keycodes
 * 
 * The tables are indexed with a 1-byte scancode, where its most significant nibble
 * corresponds to the row and its least significant nibble to the column, the code identified at
 * the intersection between row and column is the keycode that will be emitted
 * 
 * IGNR (defined as code 0x00) identifies scancodes which are ignored
*/

// Normal scancodes
static const kbd_keycode_t scantab_normal[] = {
/*       0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
/* 0 */ IGNR, 0x0A, IGNR, 0x06, 0x04, 0x02, 0x03, 0x0D, IGNR, 0x0B, 0x09, 0x07, 0x05, 0x1C, 0x0E, IGNR,
/* 1 */ IGNR, 0x46, 0x37, IGNR, 0x44, 0x1D, 0x0F, IGNR, IGNR, IGNR, 0x39, 0x2C, 0x2B, 0x1E, 0x10, IGNR,
/* 2 */ IGNR, 0x3B, 0x3A, 0x2D, 0x1F, 0x12, 0x11, IGNR, IGNR, 0x47, 0x3C, 0x2E, 0x21, 0x20, 0x13, IGNR,
/* 3 */ IGNR, 0x3E, 0x3D, 0x30, 0x2F, 0x22, 0x14, IGNR, IGNR, IGNR, 0x3F, 0x31, 0x23, 0x15, 0x16, IGNR,
/* 4 */ IGNR, 0x40, 0x32, 0x24, 0x25, 0x18, 0x17, IGNR, IGNR, 0x41, 0x42, 0x33, 0x34, 0x26, 0x19, IGNR,
/* 5 */ IGNR, IGNR, 0x35, IGNR, 0x27, 0x1A, IGNR, IGNR, 0x2A, 0x43, 0x36, 0x28, IGNR, 0x29, IGNR, IGNR,
/* 6 */ IGNR, 0x38, IGNR, IGNR, IGNR, IGNR, 0x1B, IGNR, IGNR, 0x6B, IGNR, 0x68, 0x64, IGNR, IGNR, IGNR,
/* 7 */ 0x6F, 0x70, 0x6C, 0x69, 0x6A, 0x65, 0x01, 0x60, 0x0C, 0x67, 0x6D, 0x63, 0x62, 0x66, 0x81, IGNR,
/* 8 */ IGNR, IGNR, IGNR, 0x08, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR,
/* 9 */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR,
/* A */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR,
/* B */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR,
/* C */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR,
/* D */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR,
/* E */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR,
/* F */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR,
};

// Extended scancodes (beginning with E0)
static const kbd_keycode_t scantab_extended[] = {
/*       0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
/* 0 */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR,
/* 1 */ IGNR, 0x48, IGNR, IGNR, 0x4B, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, 0x45,
/* 2 */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, 0x49, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, 0x4A,
/* 3 */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR,
/* 4 */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, 0x61, IGNR, IGNR, IGNR, IGNR, IGNR,
/* 5 */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, 0x6E, IGNR, IGNR, IGNR, IGNR, IGNR,
/* 6 */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, 0x87, IGNR, 0x8A, 0x84, IGNR, IGNR, IGNR,
/* 7 */ 0x83, 0x86, 0x8B, IGNR, 0x8C, 0x89, IGNR, IGNR, IGNR, IGNR, 0x88, IGNR, IGNR, 0x85, IGNR, IGNR,
/* 8 */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR,
/* 9 */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR,
/* A */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR,
/* B */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR,
/* C */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR,
/* D */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR,
/* E */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR,
/* F */ IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR, IGNR,
};

#endif
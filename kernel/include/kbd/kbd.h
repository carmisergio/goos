#ifndef _KBD_KBD_H
#define _KBD_KBD_H 1

#include <stdint.h>
#include <stdbool.h>

#include "kbd/codes.h"

// Key Position Code
// Representation of keys between keyboard drivers and keyboard subsystem
// Identifies every key by location
typedef uint8_t kbd_keycode_t;

// Key Code
// Representation of key presses between the keyboard subsystem and the
// keyboard event listeners
typedef uint8_t kbd_keysym_t;

// Keyboard LED states
typedef struct
{
    bool scrllck;
    bool numlck;
    bool capslck;

} kbd_led_states_t;

// State of the modifier keys
typedef struct
{
    bool shift;
    bool ctrl;
    bool alt;
    bool super;
    bool scrllck;
    bool altgr;

} kbd_mod_state_t;

// Keyboard Key event type
typedef struct
{
    kbd_keycode_t kc;
    bool make;
} kbd_key_event_t;

// Keyboard Event
typedef struct
{
    kbd_keysym_t keysym;
    kbd_mod_state_t mod;
} kbd_event_t;

/*
 * Initialize keyboard subsystem
 */
void kbd_init();

/*
 * Process key event
 * To be called from keyboard drivers
 */
void kbd_process_key_event(kbd_key_event_t e);

/*
 * Get LED states
 * Returns: current LED states
 */
kbd_led_states_t kbd_get_led_states();

/*
 * Register LED update receiver
 * Parameters:
 *   - recv: pointer to receiver function
 */
void kbd_register_led_update_recv(void (*recv)(kbd_led_states_t));

/*
 * Unregister LED update receiver
 * Parameters:
 *   - recv: pointer to receiver function
 */
void kbd_unregister_led_update_recv(void (*recv)(kbd_led_states_t));

/*
 * Register Keyboard Event receiver
 * Parameters:
 *   - recv: pointer to receiver function
 */
void kbd_register_kbd_event_recv(void (*recv)(kbd_event_t));

/*
 * Unregister Keyboard Event receiver
 * Parameters:
 *   - recv: pointer to receiver function
 */
void kbd_unregister_kbd_event_recv(void (*recv)(kbd_event_t));

#endif
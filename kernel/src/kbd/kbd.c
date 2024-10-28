#include "kbd/kbd.h"
#include "kbd/codes.h"
#include "kbd/keymap.h"

#include <stddef.h>
#include "string.h"

#include "panic.h"
#include "log.h"

#include "clock.h"

// #define DEBUG 1

// Configuration variables
#define SCRLLCK_INIT false
#define NUMLCK_INIT true
#define CAPSLCK_INIT false
#define MAX_LED_UPDATE_RECV 5
#define MAX_KBD_EVENT_RECV 5
#define DEFAULT_KEYMAP kbd_keymap_us_qwerty

#define RECV_UNUSED 0
#define KEYSTATE_BITMAP_N 256 / 8 // 8 bits in a byte

// Latching modifiers state
typedef struct
{
    bool scrllck;
    bool numlck;
    bool capslck;
} latching_mods_t;

// Internal function prototypes
static kbd_led_states_t
compute_led_states();
static void send_led_update();
static void send_kbd_event(kbd_event_t e);
static inline void keystate_bitamp_set(kbd_keycode_t kc);
static inline void keystate_bitamp_clear(kbd_keycode_t kc);
static inline bool keystate_bitamp_get(kbd_keycode_t kc);
static kbd_mod_state_t compute_mod_states();
static void process_keycode(kbd_keycode_t kc);

// Global objects
void (*led_update_recv[MAX_LED_UPDATE_RECV])(kbd_led_states_t); // Pointers to LED Update receivers
void (*kbd_event_recv[MAX_KBD_EVENT_RECV])(kbd_event_t);        // Pointers to KBD Event receivers
uint8_t keystate_bitmap[KEYSTATE_BITMAP_N];                     // State of every key as bits (1 = Down, 0 = Up)
latching_mods_t latching_mods;                                  // State of latching modifiers
kbd_keymap_t cur_keymap;                                        // Currently loaded keymap

/* Public functions */

void kbd_init()
{
    // Initialize modifier key states
    latching_mods.scrllck = SCRLLCK_INIT;
    latching_mods.numlck = NUMLCK_INIT;
    latching_mods.capslck = CAPSLCK_INIT;

    // Initialise keystate bitmap
    memset(&keystate_bitmap, 0, KEYSTATE_BITMAP_N);

    // Initialise event receivers arrays
    for (size_t i = 0; i < MAX_LED_UPDATE_RECV; i++)
        led_update_recv[i] = RECV_UNUSED;
    for (size_t i = 0; i < MAX_KBD_EVENT_RECV; i++)
        kbd_event_recv[i] = RECV_UNUSED;

    // Set  keymap
    cur_keymap = DEFAULT_KEYMAP;
}

void kbd_process_key_event(kbd_key_event_t e)
{
#ifdef DEBUG
    kprintf("[KBD] Key event: %s %x\n", e.make ? "Make" : "Break", e.kc);
#endif

    // Ignore null and ignored keys
    if (e.kc == KC_IGNR || e.kc == KC_NULL)
        return;

    // Apply patch table (if present)
    uint8_t kc = e.kc;
    if (cur_keymap.patch_map != KEYMAP_UNUSED &&
        cur_keymap.patch_map[kc] != KC_NULL)
        kc = cur_keymap.patch_map[kc];

    // Ignore null and ignored keys
    if (e.kc == KC_IGNR || e.kc == KC_NULL)
        return;

    // Update keystate bitmap
    if (e.make)
        keystate_bitamp_set(kc);
    else
        keystate_bitamp_clear(kc);

    // Handle latching modifiers
    if (e.make)
    {
        // Only modify latching state on make event
        switch (kc)
        {
        case KC_SCRLLCK:
            latching_mods.scrllck = !latching_mods.scrllck;
            send_led_update();
            return;
        case KC_NUMLCK:
            latching_mods.numlck = !latching_mods.numlck;
            send_led_update();
            return;
        case KC_CAPSLCK:
            latching_mods.capslck = !latching_mods.capslck;
            send_led_update();
            return;
        }
    }

    // Convert keycode to keysym
    if (kc != KC_NULL && e.make)
        process_keycode(kc);
}

kbd_led_states_t kbd_get_led_states()
{
    return compute_led_states();
}

void kbd_register_led_update_recv(void (*recv)(kbd_led_states_t))
{
    // Find empty slot
    for (size_t i = 0; i < MAX_LED_UPDATE_RECV; i++)
    {
        if (led_update_recv[i] == RECV_UNUSED)
        {
            // Register receiver
            led_update_recv[i] = recv;
            return;
        }
    }

    panic("KBD_REGISTER_LED_UPDATE_RECV_NO_FREE_SLOTS",
          "Tried to register a LED update handler, but no slots available");
}

void kbd_unregister_led_update_recv(void (*recv)(kbd_led_states_t))
{
    // Find slot
    for (size_t i = 0; i < MAX_LED_UPDATE_RECV; i++)
    {
        if (led_update_recv[i] == recv)
        {
            led_update_recv[i] = RECV_UNUSED;
        }
    }
}

void kbd_register_kbd_event_recv(void (*recv)(kbd_event_t))
{
    // Find empty slot
    for (size_t i = 0; i < MAX_KBD_EVENT_RECV; i++)
    {
        if (kbd_event_recv[i] == RECV_UNUSED)
        {
            // Register receiver
            kbd_event_recv[i] = recv;
            return;
        }
    }

    panic("KBD_REGISTER_KBD_EVENT_RECV_NO_FREE_SLOTS",
          "Tried to register a Keyboard Event handler, but no slots available");
}

void kbd_unregister_kbd_event_recv(void (*recv)(kbd_event_t))
{
    // Find slot
    for (size_t i = 0; i < MAX_KBD_EVENT_RECV; i++)
    {
        if (kbd_event_recv[i] == recv)
        {
            kbd_event_recv[i] = RECV_UNUSED;
        }
    }
}

/* Internal functions */

// Compute current LED states
// Returns: LED states
static kbd_led_states_t compute_led_states()
{
    kbd_led_states_t res = {
        .scrllck = latching_mods.scrllck,
        .numlck = latching_mods.numlck,
        .capslck = latching_mods.capslck};
    return res;
}

// Send LED update to registered receivers
// Gets values from current modifier states
static void send_led_update()
{
    // Construct LED states object
    kbd_led_states_t states = compute_led_states();

    // Call receivers
    for (size_t i = 0; i < MAX_LED_UPDATE_RECV; i++)
    {
        if (led_update_recv[i] != RECV_UNUSED)
            led_update_recv[i](states);
    }
}

// Send Keyboard event to listeners
// Params:
//   - e: Keyboard event
static void send_kbd_event(kbd_event_t e)
{
    // Call receivers
    for (size_t i = 0; i < MAX_KBD_EVENT_RECV; i++)
    {
        if (kbd_event_recv[i] != RECV_UNUSED)
            kbd_event_recv[i](e);
    }
}

// Set a bit in the keystate bitmap
// Params:
//   - kc: keycode to set
static inline void keystate_bitamp_set(kbd_keycode_t kc)
{
    keystate_bitmap[kc / 8] |= 0x1 << (kc % 8);
}

// Clear a bit in the keystate bitmap
// Params:
//   - kc: keycode to clear
static inline void keystate_bitamp_clear(kbd_keycode_t kc)
{
    keystate_bitmap[kc / 8] &= ~(0x1 << (kc % 8));
}

// Get state of a key in the bitmap
// Params:
//   - kc: keycode to check
// Returns: true if key is set, otherwise false
static inline bool keystate_bitamp_get(kbd_keycode_t kc)
{
    return (keystate_bitmap[kc / 8] & (0x1 << (kc % 8))) != 0;
}

// Compute current modifier key states
// Returns: current modifier states
static kbd_mod_state_t compute_mod_states()
{
    kbd_mod_state_t res;

    res.shift = keystate_bitamp_get(KC_LSHIFT) | keystate_bitamp_get(KC_RSHIFT);
    res.ctrl = keystate_bitamp_get(KC_LCTRL) | keystate_bitamp_get(KC_RCTRL);
    res.alt = keystate_bitamp_get(KC_LALT) | keystate_bitamp_get(KC_RALT);
    res.super = keystate_bitamp_get(KC_LSUPER) | keystate_bitamp_get(KC_RSUPER);
    res.altgr = keystate_bitamp_get(KC_ALTGR);
    res.scrllck = latching_mods.scrllck;

    return res;
}

// Process clean keycode after the patch map and latching modifier
// handling
static void process_keycode(kbd_keycode_t kc)
{
    kbd_keysym_t ks;

    // Get modifier states
    kbd_mod_state_t mods = compute_mod_states();

    // Apply normal map
    ks = cur_keymap.normal_map[kc];

    // Apply numlock map
    if (latching_mods.numlck && cur_keymap.numlock_map != KEYMAP_UNUSED &&
        cur_keymap.numlock_map[kc] != KS_NULL)
        ks = cur_keymap.numlock_map[kc];

    // Apply shift map
    if (mods.shift && cur_keymap.shift_map != KEYMAP_UNUSED &&
        cur_keymap.shift_map[kc] != KS_NULL)
        ks = cur_keymap.shift_map[kc];

    // Apply caps map
    if (latching_mods.capslck && cur_keymap.caps_map != KEYMAP_UNUSED &&
        cur_keymap.caps_map[ks] != KS_NULL)
        ks = cur_keymap.caps_map[ks];

    // Apply altgr map
    if (mods.altgr && cur_keymap.altgr_map != KEYMAP_UNUSED &&
        cur_keymap.altgr_map[ks] != KS_NULL)
        ks = cur_keymap.altgr_map[ks];

    if (ks == 0)
        return;

    kbd_event_t event = {
        .keysym = ks,
        .mod = mods,
    };

    send_kbd_event(event);
}
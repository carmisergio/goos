#include "drivers/ps2kbd/ps2kbd.h"
#include "scancodes.h"

#include <stdint.h>

#include "kbd/kbd.h"
#include "log.h"

#define CMD_RINGBUF_N 16
#define MAX_RESEND 5

// PS/2 command ring buffer
typedef uint8_t ps2_cmd_ringbuf_ptr;
typedef struct
{
    uint8_t data[CMD_RINGBUF_N];
    ps2_cmd_ringbuf_ptr readptr, writeptr;
} ps2_cmd_ringbuf;

// Driver state
typedef struct
{
    ps2_port port;
    ps2_cmd_ringbuf cmd_buf;
    uint16_t resend_c; // Resend counter

    // Scancode processor state machine
    bool recv_break, recv_extended;
} ps2kbd_drv_state;

// PS/2 Scan code sets
typedef enum
{
    SCAN_CODE_SET_1 = 1,
    SCAN_CODE_SET_2 = 2,
    SCAN_CODE_SET_3 = 3,
} ps2_scan_code_set;

// PS/2 Led update packet
typedef union
{
    uint8_t bits;
    struct __attribute__((packed))
    {
        bool scrollck : 1;
        bool numlck : 1;
        bool capslck : 1;
    };
} ps2_led_update_byte;

// Internal functions
static void
got_data_callback(uint8_t data);
static void process_scancode(uint8_t sc);
static void cmd_ringbuf_init(ps2_cmd_ringbuf *buf);
static bool cmd_ringbuf_is_empty(ps2_cmd_ringbuf *buf);
static bool cmd_ringbuf_is_full(ps2_cmd_ringbuf *buf);
static ps2_cmd_ringbuf_ptr cmd_ringbuf_next_ptr_val(ps2_cmd_ringbuf_ptr prev);
static void cmd_ringbuf_write(ps2_cmd_ringbuf *buf, uint8_t data);
static uint8_t cmd_ringbuf_peek(ps2_cmd_ringbuf *buf);
static void cmd_ringbuf_delete_first(ps2_cmd_ringbuf *buf);
static void send_cmd(ps2kbd_drv_state *dvr_state, uint8_t cmd);
static void write_cmd(ps2_port *port, uint8_t cmd);
static void enable_scanning(ps2kbd_drv_state *drv_state);
static void select_scan_code_set(ps2kbd_drv_state *drv_state,
                                 ps2_scan_code_set scan_code_set);
static void update_leds(ps2kbd_drv_state *drv_state,
                        kbd_led_states_t led_states);
static void led_update_recv(kbd_led_states_t states);
// Driver state
bool initialized = false;
ps2kbd_drv_state drv_state;

bool ps2kbd_init(ps2_callbacks *callbacks, ps2_port port)
{
    // Prevend double initializations
    if (initialized)
        return false;
    initialized = true;

    klog("[PS2] Initializing keyboard driver...\n");

    // Initialize driver state
    cmd_ringbuf_init(&drv_state.cmd_buf);
    drv_state.port = port;
    drv_state.recv_break = false;
    drv_state.recv_extended = false;
    drv_state.resend_c = 0;

    // Set callback for receiving data
    callbacks->got_data_callback = &got_data_callback;

    // Enable PS 2 port
    port.enable();

    // Set up keyboard
    enable_scanning(&drv_state);
    select_scan_code_set(&drv_state, SCAN_CODE_SET_2);
    update_leds(&drv_state, kbd_get_led_states());

    // Register LED update receiver
    kbd_register_led_update_recv(led_update_recv);

    return true;
}

// Data received from the PS2/ port
static void got_data_callback(uint8_t data)
{
    // Handle special bytes
    switch (data)
    {
    case 0x00: // Keyboard error
    case 0xFF:
    case 0xAA: // Self-test passed
    case 0xFD: // Self-test failed
    case 0xFC:
    case 0xEE: // Echo
        // Ignore
        return;
    case 0xFA: // Ack
        // Remove command from command buffer
        if (!cmd_ringbuf_is_empty(&drv_state.cmd_buf))
            cmd_ringbuf_delete_first(&drv_state.cmd_buf);
        // Send next command
        if (!cmd_ringbuf_is_empty(&drv_state.cmd_buf))
            write_cmd(&drv_state.port, cmd_ringbuf_peek(&drv_state.cmd_buf));
        return;
    case 0xFE: // Resend
        if (drv_state.resend_c >= MAX_RESEND)
        {
            cmd_ringbuf_delete_first(&drv_state.cmd_buf);
            drv_state.resend_c = 0;

            // Send next command
            if (!cmd_ringbuf_is_empty(&drv_state.cmd_buf))
                write_cmd(&drv_state.port, cmd_ringbuf_peek(&drv_state.cmd_buf));

            return;
        }

        drv_state.resend_c++;

        // Send last command to ring buffer
        if (!cmd_ringbuf_is_empty(&drv_state.cmd_buf))
            write_cmd(&drv_state.port, cmd_ringbuf_peek(&drv_state.cmd_buf));
        return;
    }

    // Normal Scan Code
    process_scancode(data);
}

// Process scancode from keybaord
//
// Params:
//   - sc: scancode
static void process_scancode(uint8_t sc)
{

    switch (sc)
    {
    case SC_BREAK:
        // Sequence is a break sequence
        drv_state.recv_break = true;
        return;
    case SC_EXTENDED:
        // Sequence is a break sequence
        drv_state.recv_extended = true;
        return;
    }

    // Normal scancode

    // Process scancode through scancode tables
    kbd_keycode_t kc;
    if (!drv_state.recv_extended)
        kc = scantab_normal[sc];
    else
        kc = scantab_extended[sc];

    // Ignore keys marked as ignore in the scancode tables
    if (kc == KC_IGNR || kc == KC_NULL)
        goto done;

    // Send event to keyboard subsystem for processing
    kbd_key_event_t e = {.kc = kc, .make = !drv_state.recv_break};
    kbd_process_key_event(e);

done:
    // Reset driver state
    drv_state.recv_break = false;
    drv_state.recv_extended = false;
}

// Initialize PS/2 command ring buffer
//
// Params:
//   - buf: pointer to the ring buffer
static void cmd_ringbuf_init(ps2_cmd_ringbuf *buf)
{
    buf->readptr = 0;
    buf->writeptr = 0;
}

// Checks if a PS/2 command ring buffer is empty (no data to read)
//
// Params:
//   - buf: pointer to the ring buffer
// Returns: true if empty
static bool cmd_ringbuf_is_empty(ps2_cmd_ringbuf *buf)
{
    return buf->writeptr == buf->readptr;
}

// Checks if a PS/2 command ring buffer is full (no spac to write)
//
// Params:
//   - buf: pointer to the ring buffer
// Returns: true if full
static bool cmd_ringbuf_is_full(ps2_cmd_ringbuf *buf)
{
    // If the two pointers are not absolutely aligned (buffer empty),
    // but they are pointing to the same element in the buffer,
    // the buffer is full
    return buf->writeptr != buf->readptr &&
           buf->writeptr % CMD_RINGBUF_N == buf->readptr % CMD_RINGBUF_N;
}

// Calculates the next value for the PS2/ command ring buffer pointers,
// handling wrap-arounds
//
// Params:
//   - prev: previous pointer value
// Returns: next pointer value
static ps2_cmd_ringbuf_ptr cmd_ringbuf_next_ptr_val(ps2_cmd_ringbuf_ptr prev)
{
    return (prev + 1) % (2 * CMD_RINGBUF_N);
}

// Insert value in a PS/2 command ring buffer
//
// Params:
//   - buf: pointer to the ring buffer
//   - data: data to insert
// NOTE: Caution! Doesn't do bounds checking!
static void cmd_ringbuf_write(ps2_cmd_ringbuf *buf, uint8_t data)
{
    // Write data
    buf->data[buf->writeptr % CMD_RINGBUF_N] = data;

    // Increment write pointer
    buf->writeptr = cmd_ringbuf_next_ptr_val(buf->writeptr);
}

// Peek next value from a PS/2 command ring buffer, without
// actually incrementing the read pointer
//
// Params:
//   - buf: pointer to the ring buffer
// Returns: next value
// NOTE: Caution! Doesn't do bounds checking!
static uint8_t cmd_ringbuf_peek(ps2_cmd_ringbuf *buf)
{
    // Read data
    uint8_t data = buf->data[buf->readptr % CMD_RINGBUF_N];

    return data;
}

// Increment read pointer on a PS/2 comand ring buffer
//
// Params:
//   - buf: pointer to the ring buffer
// NOTE: Caution! Doesn't do bounds checking!
static void cmd_ringbuf_delete_first(ps2_cmd_ringbuf *buf)
{
    // Increment read pointer
    buf->readptr = cmd_ringbuf_next_ptr_val(buf->readptr);
}

// Send command to the PS/2 device
// This function inserts the command into the command ring buffer
// to be sent, sending it immediately if no other command is in
// transit
//
// Params:
//   - drv_state: driver state object
//   - cmd: command to send
// Notes: ignores errors on ring buffer full
//
static void send_cmd(ps2kbd_drv_state *dvr_state, uint8_t cmd)
{
    // Check if buffer is empty prior to this insertion
    bool empty = cmd_ringbuf_is_empty(&dvr_state->cmd_buf);

    // Insert command in ring buffer
    if (!cmd_ringbuf_is_full(&dvr_state->cmd_buf))
    {
        cmd_ringbuf_write(&dvr_state->cmd_buf, cmd);

        // If buffer is empty, send command now
        if (empty)
            write_cmd(&dvr_state->port, cmd);
    }
}

// Write PS/2 command to port immediately
// Params:
//   - port: port on which to send the command
//   - cmd: command to send
static void write_cmd(ps2_port *port, uint8_t cmd)
{
    // Write byte to port
    port->send_data(cmd);
}

// Tell keyboard to enable scanning
static void enable_scanning(ps2kbd_drv_state *drv_state)
{
    send_cmd(drv_state, 0xF4);
}

// Select scan code set
static void select_scan_code_set(ps2kbd_drv_state *drv_state,
                                 ps2_scan_code_set scan_code_set)
{
    send_cmd(drv_state, 0xF0);
    send_cmd(drv_state, scan_code_set);
}

// Set LEDs state
static void update_leds(ps2kbd_drv_state *drv_state,
                        kbd_led_states_t led_states)
{

    ps2_led_update_byte data = {.scrollck = led_states.scrllck,
                                .numlck = led_states.numlck,
                                .capslck = led_states.capslck};

    // send_cmd(drv_state, 0xF5);
    send_cmd(drv_state, 0xED);
    send_cmd(drv_state, data.bits);
}

// Receives LED state updates form the keyboard subsystem
static void led_update_recv(kbd_led_states_t states)
{
    update_leds(&drv_state, states);
}
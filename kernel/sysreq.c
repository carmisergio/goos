#include "sysreq.h"

#include "kbd/kbd.h"
#include "drivers/kbdctl.h"

// Internal functions
void kbd_event_receiver(kbd_event_t e);

void sysreq_init()
{
    // Register keyboard event receiver
    kbd_register_kbd_event_recv(kbd_event_receiver);
}

void sysreq_finalize()
{
    // Unregister keyboard event receiver
    kbd_unregister_kbd_event_recv(kbd_event_receiver);
}

// Receives events from the keyboard subsystem
void kbd_event_receiver(kbd_event_t e)
{
    // Ctrl + Alt + Del
    if (e.keysym == KS_DEL && e.mod.ctrl &&
        e.mod.alt && !e.mod.shift)
        // Hard reset
        kbdctl_reset_cpu();
}
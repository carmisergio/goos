#include "sysreq.h"

#include "kbd/kbd.h"
#include "drivers/kbdctl.h"
#include "panic.h"
#include "int/interrupts.h"
#include "syscall/syscall.h"
#include "proc/proc.h"

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

    // Ctrl + Alt + Backslash
    if (e.keysym == KS_BKSLASH && e.mod.ctrl &&
        e.mod.alt && !e.mod.shift)
        // Trigger kernel panic
        panic("USER_REQUEST", "User requested kernel panic");

    // Ctrl + C
    if ((e.keysym == KS_c || e.keysym == KS_C) && e.mod.ctrl && !e.mod.alt &&
        !e.mod.shift)
    {
        // Ignore CTRL + C in init process
        if (proc_cur()->parent && proc_can_terminate())
            dishon_exit_from_int(interrupt_get_cur_ctx());
    }
}
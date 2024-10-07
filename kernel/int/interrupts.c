#include "int/interrupts.h"

#include <stdint.h>
#include "sys/io.h"

#include "int/idt.h"
#include "log.h"

void interrupts_init()
{
    klog("Initializing interrupts...\n");

    // Load IDT
    set_up_idt();

    // Enable interrupts
    asm("sti");
}

// TODO: We need to remap the PIC irqs
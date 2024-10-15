#include "clock.h"

#include "int/interrupts.h"
#include "drivers/pit.h"
#include "log.h"

// Resolution of the system clock (in milliseconds)
// Valid values:1 - 50
#define CLOCK_RESOLUTION 50 // (ms)

// Current system clock value
static volatile uint64_t system_time;

// Local time offset
static uint32_t local_time_offset;

void clock_init()
{
    cli();

    // Initialize
    system_time = 0;
    local_time_offset = 0;

    // Set up PIT ahcnnel 0 (connected to IRQ 0)
    uint16_t pit_reset = ((uint32_t)PIT_FREQ * (uint32_t)CLOCK_RESOLUTION) / 1000;
    pit_setup_channel(PIT_CHANNEL_0, PIT_MODE_3, pit_reset);

    sti();
}

uint64_t clock_get_system()
{
    return system_time;
}

uint32_t clock_get_local()
{
    return system_time / 1000 + local_time_offset;
}

void clock_set_local(uint32_t time)
{
    local_time_offset = time - system_time / 1000;
}

void clock_delay_ms(uint32_t time)
{
    uint64_t start = clock_get_system();

    while (clock_get_system() - start < time)
        ;
}

void clock_handle_timer_irq()
{
    // Each tick is (CLOCK_RESOLUTION) milliseconds long
    system_time += CLOCK_RESOLUTION;
}
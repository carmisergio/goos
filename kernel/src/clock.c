#include "clock.h"

#include <stddef.h>
#include "sync.h"

#include "int/interrupts.h"
#include "drivers/pit.h"
#include "log.h"
#include "cpu.h"

// Resolution of the system clock (in milliseconds)
// Valid values:1 - 50
#define CLOCK_RESOLUTION 50 // (ms)

#define N_TIMERS 16

#define TIMER_IRQ 0

// Representation of a timer
typedef struct timer timer_t;
struct timer
{
    bool used;
    timer_handle_t handle;
    timer_type_t type;
    uint64_t duration;
    uint64_t start;

    // Callback
    void (*func)(void *);
    void *data;
};

// Internal function prototypes
static void clock_handle_timer_irq();
static void process_timers();
static timer_t *find_timer_by_handle(timer_handle_t handle);

// Global objects
static volatile uint64_t system_time;
static uint32_t local_time_offset;
static slock_t timers_lck;
static timer_t timers[N_TIMERS];
static timer_handle_t next_timer_handle;

/* Public functions */

void clock_init()
{
    // Initialize
    system_time = 0;
    local_time_offset = 0;

    // Set up PIT ahcnnel 0 (connected to IRQ 0)
    uint16_t pit_reset = ((uint32_t)PIT_FREQ * (uint32_t)CLOCK_RESOLUTION) / 1000;
    pit_setup_channel(PIT_CHANNEL_0, PIT_MODE_3, pit_reset);

    // Lock timers
    slock_init(&timers_lck);
    slock_acquire(&timers_lck);

    // Register timer IRQ handler
    interrupts_register_irq(TIMER_IRQ, clock_handle_timer_irq);

    // Start from timer handle 0
    next_timer_handle = 0;

    // Initialize timers list
    for (size_t i = 0; i < N_TIMERS; i++)
        timers[i].used = false;

    // Release timers
    slock_release(&timers_lck);
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
        pause();
}

timer_handle_t clock_set_timer(uint64_t duration, timer_type_t type,
                               void (*func)(void *), void *data)
{
    timer_handle_t handle = -1;

    slock_acquire(&timers_lck);

    for (size_t i = 0; i < N_TIMERS; i++)
    {
        // Find unused timer
        if (!timers[i].used)
        {
            handle = next_timer_handle;
            next_timer_handle++;

            timers[i].used = true;
            timers[i].duration = duration;
            timers[i].handle = handle;
            timers[i].type = type;
            timers[i].func = func;
            timers[i].data = data;
            timers[i].start = system_time;

            break;
        }
    }

    slock_release(&timers_lck);

    return handle;
}

void clock_clear_timer(timer_handle_t handle)
{
    timer_t *timer;
    slock_acquire(&timers_lck);

    if ((timer = find_timer_by_handle(handle)) != NULL)
        timer->used = false;

    slock_release(&timers_lck);
}

bool clock_reset_timer(timer_handle_t handle, uint64_t duration)
{
    timer_t *timer;
    bool reset_ok = false;
    slock_acquire(&timers_lck);

    if ((timer = find_timer_by_handle(handle)) != NULL)
    {
        timer->duration = duration;
        timer->start = system_time;
        reset_ok = true;
    }

    slock_release(&timers_lck);

    return reset_ok;
}

bool clock_is_timer_active(timer_handle_t handle)
{
    timer_t *timer;
    bool res = false;

    slock_acquire(&timers_lck);

    if ((timer = find_timer_by_handle(handle)) != NULL)
        res = true;

    slock_release(&timers_lck);

    return res;
}

/* Internal functions */

static void clock_handle_timer_irq()
{
    // Each tick is (CLOCK_RESOLUTION) milliseconds long
    system_time += CLOCK_RESOLUTION;

    process_timers();
}

static void process_timers()
{
    // Try to acquire a lock on the event list
    // Don't process if unable
    if (!slock_try_acquire(&timers_lck))
        return;

    // Process timers
    for (size_t i = 0; i < N_TIMERS; i++)
    {
        // Only process used timers
        if (!timers[i].used)
            continue;

        // Check if duration is elapsed
        if (system_time - timers[i].start < timers[i].duration)
            continue;

        // Call callback function
        timers[i].func(timers[i].data);

        if (timers[i].type == TIMER_ONESHOT)
            // Delete oneshot timer
            timers[i].used = false;
        else
            // Restart timer
            timers[i].start = system_time;
    }

    slock_release(&timers_lck);
}

// Look up timer in the timer array by its handle
// Returns null if not found
static timer_t *find_timer_by_handle(timer_handle_t handle)
{
    for (size_t i = 0; i < N_TIMERS; i++)
    {
        if (timers[i].used && timers[i].handle == handle)
            return &timers[i];
    }

    return NULL;
}
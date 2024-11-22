#include "sync.h"

#include "int/interrupts.h"

void slock_init(slock_t *l)
{
    *l = false;
}

void slock_release(slock_t *l)
{
    cli();

    *l = false;

    sti();
}

void slock_acquire(slock_t *l)
{
    while (!slock_try_acquire(l))
        ;
}

bool slock_try_acquire(slock_t *l)
{
    bool res = false;
    cli();

    if (!*l)
    {
        *l = true;
        res = true;
    }

    sti();
    return res;
}

bool slock_peek(slock_t *l)
{
    return *l;
}
#include "time.h"

#include "goos.h"

time_t time(time_t *tloc)
{
    tloc = tloc; // Ignore
    return _g_get_local_time();
}

void sleep(time_t time)
{
    _g_delay_ms(time * 1000);
}

void msleep(uint32_t ms)
{
    _g_delay_ms(ms);
}
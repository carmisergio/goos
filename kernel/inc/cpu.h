#ifndef _CPU
#define _CPU 1

static inline void pause()
{
    __asm__("pause");
}

#endif
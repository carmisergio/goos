#ifndef _CPU
#define _CPU 1

#define EFLAGS 0x2 // Default info
#define EFLAGS_IF (1 << 9)

static inline void pause()
{
    __asm__("pause");
}

#endif
// Address conversion macros
#ifdef __ASSEMBLER__
#define V2P(a) ((a) - KERNEL_OFFSET)
#define P2V(a) ((a) + KERNEL_OFFSET)
#else
#include <stdint.h>
#define V2P(a) ((uintptr_t)(a) & ~KERNEL_OFFSET)
#define P2V(a) ((uintptr_t)(a) | KERNEL_OFFSET))
#endif
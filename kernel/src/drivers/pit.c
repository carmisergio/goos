#include "drivers/pit.h"

#include "sys/io.h"

#include "int/interrupts.h"

// PIT IO ports
#define PIT_BASE 0x40
#define PIT_CMD 0x43

// Access modes
#define ACCESS_MODE_LO 0b01
#define ACCESS_MODE_HI 0b10
#define ACCESS_MODE_LOHI 0b11
#define ACCESS_MODE_LATCH 0b00

typedef union
{
    struct
    {
        uint8_t bcd : 1;
        uint8_t operating_mode : 3;
        uint8_t access_mode : 2;
        uint8_t channel : 2;
    };
    uint8_t bits;
} pit_init_word_t;

static inline void pit_init_channel(pit_channel_t channel, pit_mode_t mode)
{
    pit_init_word_t cmd = {.channel = channel,
                           .access_mode = ACCESS_MODE_LOHI,
                           .operating_mode = mode,
                           .bcd = 0};

    // Send initialization command
    outb(PIT_CMD, cmd.bits);
}

void pit_setup_channel(pit_channel_t channel, pit_mode_t mode, uint16_t reset)
{
    // Disable interrupts
    cli();

    // Send initialization word
    pit_init_channel(channel, mode);

    // Send reset value
    outb16_lh(PIT_BASE + channel, reset);

    // Enable interrupts
    sti();
}
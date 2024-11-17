#include "drivers/isadma.h"

#include <stddef.h>
#include "sys/io.h"

#include "sync.h"
#include "log.h"

#define DMA_CHAN_MAX DMA_CHAN3

// I/O ports of DMA controlloer 0
enum DMA0_IO
{

    DMA0_STATUS_REG = 0x08,
    DMA0_COMMAND_REG = 0x08,
    DMA0_REQUEST_REG = 0x09,
    DMA0_CHANMASK_REG = 0x0A,
    DMA0_MODE_REG = 0x0B,
    DMA0_CLEARBYTE_FLIPFLOP_REG = 0x0C,
    DMA0_TEMP_REG = 0x0D,
    DMA0_MASTER_CLEAR_REG = 0x0D,
    DMA0_CLEAR_MASK_REG = 0x0E,
    DMA0_MASK_REG = 0x0F,
    DMA0_CHAN0_ADDR_REG = 0x00,
    DMA0_CHAN0_COUNT_REG = 0x01,
    DMA0_CHAN1_ADDR_REG = 0x02,
    DMA0_CHAN1_COUNT_REG = 0x03,
    DMA0_CHAN2_ADDR_REG = 0x04,
    DMA0_CHAN2_COUNT_REG = 0x05,
    DMA0_CHAN3_ADDR_REG = 0x06,
    DMA0_CHAN3_COUNT_REG = 0x07,
    DMA0_CHAN1_EXTPAGE_REG = 0x83,
    DMA0_CHAN2_EXTPAGE_REG = 0x81,
    DMA0_CHAN3_EXTPAGE_REG = 0x82,
    DMA0_RESET = 0x0C,
};

// // I/O ports of DMA controlloer 1
// enum DMA1_IO
// {

//     DMA1_STATUS_REG = 0xD0,
//     DMA1_COMMAND_REG = 0xD0,
//     DMA1_REQUEST_REG = 0xD2,
//     DMA1_CHANMASK_REG = 0xD4,
//     DMA1_MODE_REG = 0xD6,
//     DMA1_CLEARBYTE_FLIPFLOP_REG = 0xD8,
//     DMA1_INTER_REG = 0xDA,
//     DMA1_UNMASK_ALL_REG = 0xDC,
//     DMA1_MASK_REG = 0xDE,
//     DMA1_CHAN4_ADDR_REG = 0xC0,
//     DMA1_CHAN4_COUNT_REG = 0xC2,
//     DMA1_CHAN5_ADDR_REG = 0xC4,
//     DMA1_CHAN5_COUNT_REG = 0xC6,
//     DMA1_CHAN6_ADDR_REG = 0xC8,
//     DMA1_CHAN6_COUNT_REG = 0xCA,
//     DMA1_CHAN7_ADDR_REG = 0xCC,
//     DMA1_CHAN7_COUNT_REG = 0xCE,

// };

// Internal function prototypes
static void set_mask(isadma_chan_t chan, bool state);
static void set_address(isadma_chan_t chan, void *addr);
static void set_address_reg(isadma_chan_t chan, uint16_t addr);
static void set_extpage_reg(isadma_chan_t chan, uint8_t page);
static void set_count(isadma_chan_t chan, uint16_t count);
static void set_mode(isadma_chan_t chan, isadma_tt_t tt, isadma_tm_t tm,
                     bool autoinit);
static void reset_flipflop();
static void reset_controller();

// Gloal objects
slock_t channel_lck[DMA_CHAN_MAX + 1]; // Channel locks

void isadma_init()
{
    // Initialize channel locks
    for (size_t i = 0; i <= DMA_CHAN_MAX; i++)
        slock_init(&channel_lck[i]);

    // Reset DMA controller
    reset_controller();

    // Mask all channels
    outb(DMA0_MASK_REG, 0);
    // NOTE: we could need to unmask channel 4 as it is the cascad channel
}

void isadma_setup_channel(isadma_chan_t chan, void *base_paddr,
                          uint16_t count, isadma_tt_t tt, isadma_tm_t tm,
                          bool autoinit)
{
    // Acquire channel lock
    slock_acquire(&channel_lck[chan]);

    // Mask channel
    set_mask(chan, true);

    // Setup registers
    set_address(chan, base_paddr);
    set_count(chan, count - 1);
    set_mode(chan, tt, tm, autoinit);

    // Unmask channel
    set_mask(chan, false);
}

void isadma_release_channel(isadma_chan_t chan)
{
    // Mask the channel
    set_mask(chan, true);

    // Release channel lock
    slock_release(&channel_lck[chan]);
}

/* Internal functions */

// Set mask value for one channel
// state: true = masked, false = unmasked
static void set_mask(isadma_chan_t chan, bool state)
{
    uint8_t val = (chan & 0b11) | state ? 0b000 : 0b100;
    outb(DMA0_CHANMASK_REG, val);
}

// Set base address
// Automatically sets page registers as well
static void set_address(isadma_chan_t chan, void *addr)
{
    uint16_t base_addr = (uint32_t)addr & 0xFFFF;
    uint8_t page = ((uint32_t)addr >> 16) & 0xFF;

    // Set registers
    set_address_reg(chan, base_addr);
    set_extpage_reg(chan, page);
}

// Set base address register
static void set_address_reg(isadma_chan_t chan, uint16_t addr)
{
    reset_flipflop();

    // Get address register value
    uint16_t reg;
    switch (chan)
    {
    case DMA_CHAN1:
        reg = DMA0_CHAN1_ADDR_REG;
        break;
    case DMA_CHAN2:
        reg = DMA0_CHAN2_ADDR_REG;
        break;
    case DMA_CHAN3:
        reg = DMA0_CHAN3_ADDR_REG;
        break;
    default:
        return;
    }

    // Send two bytes in sequence
    outb16_lh(reg, addr);
}

// Set external page register
static void set_extpage_reg(isadma_chan_t chan, uint8_t page)
{
    // Get page register value
    uint16_t reg;
    switch (chan)
    {
    case DMA_CHAN1:
        reg = DMA0_CHAN1_EXTPAGE_REG;
        break;
    case DMA_CHAN2:
        reg = DMA0_CHAN2_EXTPAGE_REG;
        break;
    case DMA_CHAN3:
        reg = DMA0_CHAN3_EXTPAGE_REG;
        break;
    default:
        return;
    }

    outb(reg, page);
}

// Set transfer count
static void set_count(isadma_chan_t chan, uint16_t count)
{
    reset_flipflop();

    uint16_t reg;
    switch (chan)
    {
    case DMA_CHAN1:
        reg = DMA0_CHAN1_COUNT_REG;
        break;
    case DMA_CHAN2:
        reg = DMA0_CHAN2_COUNT_REG;
        break;
    case DMA_CHAN3:
        reg = DMA0_CHAN3_COUNT_REG;
        break;
    default:
        return;
    }

    // Send two bytes in sequence
    outb16_lh(reg, count);
}

// Set DMA mode
static void set_mode(isadma_chan_t chan, isadma_tt_t tt, isadma_tm_t tm,
                     bool autoinit)
{
    uint8_t val = chan |
                  tt << 2 |
                  (autoinit ? 1 : 0) << 4 |
                  tm << 6;

    outb(DMA0_MODE_REG, val);
}

// Reset flipflop on the controller
static void reset_flipflop()
{
    outb(DMA0_CLEARBYTE_FLIPFLOP_REG, 0xFF); // Value is irrelevant
}

// Reset DMA controller
static void reset_controller()
{
    outb(DMA0_RESET, 0xFF);
}
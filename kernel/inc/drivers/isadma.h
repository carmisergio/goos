#pragma once

#include <stdbool.h>
#include <stdint.h>

// DMA channels
typedef enum
{
    DMA_CHAN1 = 1,
    DMA_CHAN2 = 2,
    DMA_CHAN3 = 3,

    // We don't currently support 16 bit DMA channels
    // DMA_CHAN5 = 5,
    // DMA_CHAN6 = 6,
    // DMA_CHAN7 = 7,
} isadma_chan_t;

// DMA transfer type
typedef enum
{
    DMA_TOMEM = 0x1,   // Transfer TO memory
    DMA_FROMMEM = 0x2, // Transfer FROM memory
} isadma_tt_t;

// DMA transfer mode
typedef enum
{
    DMA_ONDEMAND = 0x0, // On Demand DMA transfer
    DMA_SINGLE = 0x1,   // Single DMA transfer
    DMA_BLOCK = 0x2,    // Block DMA transfer
} isadma_tm_t;

/*
 * Initialize DMA management
 */
void isadma_init();

/*
 * Set up DMA channel for transfer
 * #### Parameters:
 *   - chan: channel
 *   - base_paddr: transfer base physical address
 *   - count: transfer count in bytes
 *   - tt: transfer type
 *   - tm: transfer mode
 *   - autoinit: enable auto re-initialization
 *  NOTE: the buffer assigned to the transfer MUST be
 *        below 16M and MUST NOT cross a 64K boundary
 */
void isadma_setup_channel(isadma_chan_t chan, void *base_paddr,
                          uint16_t count, isadma_tt_t tt, isadma_tm_t tm,
                          bool autoinit);

/*
 * Make DMA channel available to other transfers
 */
void isadma_release_channel(isadma_chan_t chan);
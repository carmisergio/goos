#include "drivers/ramdisk.h"

#include "string.h"
#include "mini-printf.h"

#include "blkdev/blkdev.h"
#include "mem/kalloc.h"
#include "log.h"

typedef struct
{
    uint8_t **blklst;
    uint32_t nblocks;
} rd_state_t;

// Internal function prototypes
static uint8_t **allocate_blocklist(uint32_t nblocks);
static bool blkdev_read_blk_req(blkdev_t *dev, uint8_t *buf, uint32_t block);
static bool write_req(blkdev_t *dev, const uint8_t *buf, uint32_t block);
void ramdisk_create(uint32_t id, uint32_t nblocks)
{
    // Construct major
    size_t major_len = snprintf(NULL, 0, "rd%d", id) + 1;
    char *major = kalloc(major_len);
    snprintf(major, major_len, "rd%d", id);

    // Allocate driver state
    rd_state_t *state = kalloc(sizeof(kfree));
    if (state == NULL)
    {
        kprintf("[RAMDISK] %d creation failure: Not enough memory\n", id);
        return;
    }

    // Allocate blocklist
    uint8_t **blklst = allocate_blocklist(nblocks);
    if (blklst == NULL)
    {
        kprintf("[RAMDISK] %d creation failure: Not enough memory\n", id);

        // Deallocate driver state
        kfree(state);

        return;
    }

    state->nblocks = nblocks;
    state->blklst = blklst;

    // Construct device struct
    blkdev_t dev = {
        .major = major,
        .drvstate = state,
        .nblocks = nblocks,
        .read_blk = blkdev_read_blk_req,
        .write_blk = write_req,
        .media_changed = NULL,
    };

    // Register block device
    blkdev_register(dev);
}

/* Internal functions */

// Allocate blocks
static uint8_t **allocate_blocklist(uint32_t nblocks)
{
    // Allocate list of pointers to blocks
    uint8_t **blklist = kalloc(sizeof(uint8_t *) * nblocks);

    if (blklist == NULL)
        return NULL;

    // Allocate blocks
    for (size_t i = 0; i < nblocks; i++)
    {
        if ((blklist[i] = kalloc(BLOCK_SIZE)) == NULL)
        {
            // Deallocate previously allocated blocks
            for (size_t j = 0; j < i; j++)
                kfree(blklist[i]);

            // Deallocate blocklist
            kfree(blklist);

            return NULL;
        }
    }

    return blklist;
}

static bool blkdev_read_blk_req(blkdev_t *dev, uint8_t *buf, uint32_t block)
{
    rd_state_t *state = (rd_state_t *)dev->drvstate;

    // Check if block in range
    if (block >= state->nblocks)
        return false;

    memcpy(buf, state->blklst[block], BLOCK_SIZE);

    return true;
}

static bool write_req(blkdev_t *dev, const uint8_t *buf, uint32_t block)
{
    rd_state_t *state = (rd_state_t *)dev->drvstate;

    // Check if block in range
    if (block >= state->nblocks)
        return false;

    memcpy(state->blklst[block], buf, BLOCK_SIZE);

    return true;
}

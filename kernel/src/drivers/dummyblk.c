#include "drivers/dummyblk.h"

#include "mini-printf.h"

#include "blkdev/blkdev.h"
#include "mem/kalloc.h"
#include "log.h"

// Internal function prototypes
bool read_op(void *data, block_buf_t *buf, uint32_t block);

void dummyblk_init(uint32_t id, uint32_t nblocks)
{
    // Get major
    size_t major_len = snprintf(NULL, 0, "d%d", id) + 1;
    char *major = kalloc(major_len);
    kprintf("Size: %d\n", major_len);
    snprintf(major, major_len, "d%d", id);

    // Id size sanity check
    _Static_assert(sizeof(uint32_t) <= sizeof(void *), "id too big");

    // Construct device struct
    blkdev_t dev = {
        .major = major,
        .drvstate = (void *)id,
        .nblocks = nblocks,
        .read_blk = read_op,
        .media_changed = NULL,
    };

    // Register block device
    blkdev_register(dev);
}

bool read_op(void *data, block_buf_t *buf, uint32_t block)
{
    for (size_t i = 0; i < BLOCK_SIZE; i += 2 * sizeof(uint32_t))
    {
        *(uint32_t *)((char *)buf + i) = (uint32_t)data;
        *(uint32_t *)((char *)buf + i + sizeof(uint32_t)) = block;
    }
    return true;
}
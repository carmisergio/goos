#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define BLOCK_SIZE 512 // bytes

// Block device handle type
typedef size_t blkdev_handle_t;
#define BLKDEV_HANDLE_NULL 0

// Block buffer
typedef uint8_t block_buf_t;

// Representation of a block device
typedef struct
{
    // Device info
    char *major;      // Name of block device
    uint32_t nblocks; // Size (number of blocks)

    // Driver state (will be passed to all functions)
    void *drvstate;

    /// Operations
    // Read block
    bool (*read_blk)(void *, block_buf_t *, uint32_t); // (drvstate, buffer, intval) -> success

    // Check if media was changed
    bool (*media_changed)(void *); // (drvstate) -> true: media chagned
} blkdev_t;

// Initialize block device subsystem
void blkdev_init();

/*
 * Register new block device
 * This function is called by drivers to make a new block device available
 * to the subsystem
 */
bool blkdev_register(blkdev_t dev);

/*
 * Get handle to block device
 * #### Parameters:
 *   - name: name of block device
 * #### Returns:
 *   device handle, BLKDEV_HANDLE_NULL on failure
 */
blkdev_handle_t blkdev_get_handle(const char *major);

/*
 * Read block from block device
 * #### Parameters:
 *   - buf: will set this to a pointer to a buffer containing
 *          the requested block's data
 *   - handle: block device handle
 *   - block: logical block ID to read
 */
bool blkdev_read(block_buf_t **buf, const blkdev_handle_t handle,
                 const uint32_t block);

// Debug registered devices
void blkdev_debug_devices();

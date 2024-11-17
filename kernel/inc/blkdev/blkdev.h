#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define BLOCK_SIZE 512 // bytes

// Block device handle type
typedef size_t blkdev_handle_t;
#define BLKDEV_HANDLE_NULL 0

// Representation of a block device
typedef struct _blkdev_t
{
    // Device info
    char *major;      // Name of block device
    uint32_t nblocks; // Size (number of blocks)

    // Private driver state
    void *drvstate;

    /// Operations
    // Read block
    bool (*read_blk)(struct _blkdev_t *, uint8_t *, uint32_t); // (drvstate, buffer, blkid) -> success
    // Write block
    bool (*write_blk)(struct _blkdev_t *, const uint8_t *, uint32_t); // (drvstate, buffer, blkid) -> success
    // Check if media was changed
    bool (*media_changed)(struct _blkdev_t *); // (drvstate) -> true: media chagned
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
 * Release handle to block device
 * #### Parameters:
 *  - handle: block device handle
 */
void blkdev_release_handle(blkdev_handle_t handle);

/*
 * Read block from block device
 * #### Parameters:
 *   - buf: buffer to read into
 *   - handle: block device handle
 *   - block: logical block ID to read
 * #### Returns: true on success
 */
bool blkdev_read(uint8_t *buf, const blkdev_handle_t handle,
                 const uint32_t block);

/*
 * Read n contiguous blocks from block device
 * #### Parameters:
 *   - buf: buffer to read into
 *   - handle: block device handle
 *   - start: logical block ID to start reading from
 *   - n: number of blocks to read
 * #### Returns: true on success
 */
bool blkdev_read_n(uint8_t *buf, const blkdev_handle_t handle,
                   const uint32_t start, const uint32_t n);

/*
 * Write block to block device
 * #### Parameters:
 *   - buf: buffer to write
 *   - handle: block device handle
 *   - block: logical block ID to write
 * #### Returns: true on success
 */
bool blkdev_write(const uint8_t *buf, const blkdev_handle_t handle,
                  const uint32_t block);

/*
 * Check if block device media was changed
 * #### Parameters:
 *   - handle: block device handle
 * #### Returns: true if media changed
 */
bool blkdev_media_changed(const blkdev_handle_t handle);

// Debug registered devices
void blkdev_debug_devices();

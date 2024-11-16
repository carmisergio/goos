#include "blkdev/blkdev.h"

#include "config.h"
#include "string.h"
#include "collections/dllist.h"
#include "mem/kalloc.h"
#include "panic.h"
#include "log.h"

// Configure debugging
#if DEBUG_BLKDEV == 1
#define DEBUG
#endif

// Maximum number of active block device handles
#define MAX_HANDLES 4

// Node in the device list
typedef struct
{
    blkdev_t dev;

    // There currently is a handle to this device
    bool used;
} devlst_entry_t;

// Device handle slot
typedef struct
{
    devlst_entry_t *devlst_entry;
    bool used;
} handle_slot_t;

// Internal functions
devlst_entry_t *find_by_major(const char *major);
blkdev_handle_t find_handle();
static inline size_t handle_to_index(const blkdev_handle_t handle);
static inline blkdev_handle_t slot_to_handle(size_t idx);

// Global objects
dllist_t dev_list;                  // Registered devices list
handle_slot_t handles[MAX_HANDLES]; // Block device handles

void blkdev_init()
{
    // Initialize device list
    dllist_init(&dev_list);
}

bool blkdev_register(blkdev_t dev)
{
    // Check if device major already exists
    if (find_by_major(dev.major) != NULL)
        return false;

    // Add device to the list
    devlst_entry_t *entry = kalloc(sizeof(devlst_entry_t));
    if (entry == NULL)
        return false;

    // Set entry
    entry->dev = dev;
    entry->used = false;

    // Add to device list
    dllist_insert_tail(&dev_list, (void *)entry);

    kprintf("[BLKDEV] Device registered: %s (%d blocks)\n", dev.major, dev.nblocks);

    return true;
}

blkdev_handle_t blkdev_get_handle(const char *major)
{
    // Find device by major
    devlst_entry_t *dev = find_by_major(major);
    if (dev == NULL)
        return BLKDEV_HANDLE_NULL;

    // Check if device is available
    if (dev->used)
        return BLKDEV_HANDLE_NULL;

    // Find available handle
    blkdev_handle_t handle = find_handle();
    if (handle == BLKDEV_HANDLE_NULL)
        return BLKDEV_HANDLE_NULL;

    size_t idx = handle_to_index(handle);
    handles[idx].used = true;
    handles[idx].devlst_entry = dev;
    dev->used = true;

#ifdef DEBUG
    kprintf("[BLKDEV] Got handle for device %s: %d\n", major, handle);
#endif

    return handle;
}

void blkdev_release_handle(blkdev_handle_t handle)
{
    if (handle == BLKDEV_HANDLE_NULL)
        return;

    size_t idx = handle_to_index(handle);
    handles[idx].used = false;
    handles[idx].devlst_entry->used = false;

#ifdef DEBUG
    kprintf("[BLKDEV] Released handle %d\n", handle);
#endif
}

bool blkdev_read(uint8_t *buf, const blkdev_handle_t handle,
                 const uint32_t block)
{
    if (handle == BLKDEV_HANDLE_NULL)
        return false;

    size_t idx = handle_to_index(handle);

    // Check handle validity
    if (idx >= MAX_HANDLES || !handles[idx].used)
        return false;

    // Get device
    blkdev_t *dev = &handles[idx].devlst_entry->dev;

#ifdef DEBUG
    kprintf("[BLKDEV] Device %s (handle %d), read block %d\n", dev->major, handle, block);
#endif

    // Check block in range
    if (block >= dev->nblocks)
        return false;

    // Check if device supports reading
    if (dev->read_blk == NULL)
        return false;

    // Perform read operation
    return dev->read_blk(dev, buf, block);
}

bool blkdev_read_n(uint8_t *buf, const blkdev_handle_t handle,
                   const uint32_t start, const uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
    {
        if (!blkdev_read(buf + i * BLOCK_SIZE, handle, start + i))
            return false;
    }

    return true;
}

bool blkdev_write(const uint8_t *buf, const blkdev_handle_t handle,
                  const uint32_t block)
{
    if (handle == BLKDEV_HANDLE_NULL)
        return false;

    size_t idx = handle_to_index(handle);

    // Check handle validity
    if (idx >= MAX_HANDLES || !handles[idx].used)
        return false;

    // Get device
    blkdev_t *dev = &handles[idx].devlst_entry->dev;

#ifdef DEBUG
    kprintf("[BLKDEV] Device %s (handle %d), write block %d\n", dev->major, handle, block);
#endif

    // Check block in range
    if (block >= dev->nblocks)
        return false;

    // Check if device supports writing
    if (dev->write_blk == NULL)
        return false;

    // Perform read operation
    return dev->write_blk(dev, buf, block);
}

bool blkdev_media_changed(const blkdev_handle_t handle)
{
    if (handle == BLKDEV_HANDLE_NULL)
        return false;

    size_t idx = handle_to_index(handle);

    // Check handle validity
    if (idx >= MAX_HANDLES || !handles[idx].used)
        return false;

    // Get device
    blkdev_t *dev = &handles[idx].devlst_entry->dev;

    // Check if device supports writing
    if (dev->media_changed == NULL)
        return false;

    // Perform read operation
    return dev->media_changed(dev);
}

/* Internal functions */
devlst_entry_t *find_by_major(const char *major)
{
    dllist_node_t *cur = dllist_head(&dev_list);
    while (cur != NULL)
    {
        devlst_entry_t *entry = dllist_data(cur);

        if (strcmp(entry->dev.major, major) == 0)
        {
            // Found device
            return entry;
        }

        cur = dllist_next(cur);
    }

    // No device found
    return NULL;
}

blkdev_handle_t find_handle()
{
    for (size_t i = 0; i < MAX_HANDLES; i++)
    {
        if (!handles[i].used)
            return slot_to_handle(i);
    }

    return BLKDEV_HANDLE_NULL;
}

static inline size_t handle_to_index(const blkdev_handle_t handle)
{
    return handle - 1;
}

static inline blkdev_handle_t slot_to_handle(size_t idx)
{
    return idx + 1;
}

void blkdev_debug_devices()
{
    kprintf("[BLKDEV] Registered devices:\n");
    dllist_node_t *cur = dllist_head(&dev_list);
    while (cur != NULL)
    {
        devlst_entry_t *entry = dllist_data(cur);

        kprintf(" - %s (%d blocks)\n", entry->dev.major, entry->dev.nblocks);

        cur = dllist_next(cur);
    }
}
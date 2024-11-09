#include "fs/vfs.h"

#include <stdbool.h>
#include <stddef.h>

#define MAX_MOUNT_POINTS 16

// Represents a mountpoint
typedef struct
{
    bool used;         // Mountpoint used
    vfs_mount_t *mount // Mounted filesystem
} mount_point_t;

// Global objects
mount_point_t mountpoints[MAX_MOUNT_POINTS];

void vfs_init()
{
    // Initialize mountpoint list
    for (size_t i = 0; i < MAX_MOUNT_POINTS; i++)
        mountpoints[i].used = false;
}
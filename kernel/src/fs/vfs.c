#include "fs/vfs.h"

#include "collections/dllist.h"
#include "string.h"

#include "mem/kalloc.h"

#define MAX_MOUNT_POINTS 16

// Represents a mountpoint
typedef struct
{
    bool used;              // Mountpoint used
    vfs_superblock_t *mount // Mounted filesystem
} mount_point_slot_t;

// Internal functions
vfs_fs_type_t *find_fs_type(const char *name);

// Global objects
dllist_t fs_types;
mount_point_slot_t mountpoints[MAX_MOUNT_POINTS];

void vfs_init()
{
    // Initialize mountpoint list
    for (size_t i = 0; i < MAX_MOUNT_POINTS; i++)
        mountpoints[i].used = false;

    // Initialize fs types list
    dllist_init(&fs_types);
}

bool vfs_register_fs_type(vfs_fs_type_t fs_type)
{
    // Allocate space for new FS type
    vfs_fs_type_t *fs_type_ptr = kalloc(sizeof(vfs_fs_type_t));
    if (!fs_type_ptr)
        return false;

    // Copy data
    *fs_type_ptr = fs_type;

    // Insert fs type in fs type list
    dllist_insert_tail(&fs_types, (void *)fs_type_ptr);

    return true;
}

bool vfs_mount(char *dev, mount_point_t mp, char *fs)
{
    // Check if mountpoint is valid and not already mounted
    if (mp >= MAX_MOUNT_POINTS || mountpoints[mp].used)
        return false;

    // Find correct FS driver
    vfs_fs_type_t *fs_type = find_fs_type(fs);
    if (!fs_type)
        return false;

    // Mount
    vfs_superblock_t *mount;
    if (!fs_type->mount(dev, &mount))
        return false;

    // Set mount point
    mountpoints[mp].mount = mount;
    mountpoints[mp].used = true;

    return true;
}

bool vfs_unmount(mount_point_t mp)
{
    // Check if mountpoint is valid and mounted
    if (mp >= MAX_MOUNT_POINTS || !mountpoints[mp].used)
        return false;

    // Unmount
    mountpoints[mp].mount->unmount(mountpoints[mp].mount);

    // Free mount slot
    mountpoints[mp].mount = NULL;
    mountpoints[mp].used = false;
}

/* Internal functions */
vfs_fs_type_t *find_fs_type(const char *name)
{
    dllist_node_t *cur = dllist_head(&fs_types);
    while (cur != NULL)
    {
        vfs_fs_type_t *entry = dllist_data(cur);

        if (strcmp(entry->name, name) == 0)
        {
            // Found device
            return entry;
        }

        cur = dllist_next(cur);
    }

    // No device found
    return NULL;
}
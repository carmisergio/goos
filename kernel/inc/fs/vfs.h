#pragma once

#include <stdint.h>

// Inode type
typedef enum
{
    VFS_INTYPE_FILE,
    VFS_INTYPE_DIR,
} vfs_inode_type_t;

// VFS inode
// Represents a file in the virtual file system
typedef struct _vfs_inode_t vfs_inode_t;
struct _vfs_inode_t
{
    char *name;            // Filename
    vfs_inode_type_t type; // Type of inode
    void *priv_data;       // Filesystem private data
    void *fs_state;        // Mountpoint private state

    // Read data from inode
    // uint32_t read(vfs_inode_t *inode, uint32_t offset, uint32_t length, uint8_t *buf)
    uint32_t (*read)(vfs_inode_t *, uint32_t, uint32_t, uint8_t *);

    // Write data to inode
    // uint32_t write(vfs_inode_t *inode, uint32_t offset, uint32_t length, uint8_t *buf)
    uint32_t (*write)(vfs_inode_t *, uint32_t, uint32_t, uint8_t *);

    // Lookup child in directory inode by name
    // vfs_inode_t *lookup(vfs_inode_t *inode, char *name)
    vfs_inode_t (*lookup)(vfs_inode_t *, char *);

    // List child of directory inode by index
    // Is used to retreive the listing of a directory, by repetedly calling this method
    // with increaseing index until it returns NULL
    // vfs_inode_t *nthchild(vfs_inode_t *inode, uint32-t index)    `
    vfs_inode_t (*nthchild)(vfs_inode_t *, uint32_t);

    // Destroy inode
    // Deallocate inode and any private data
    // void destroy(vfs_inode_t *inode)
    void (*destroy)(vfs_inode_t *);
};

// Filesystem type
// Represents a filesystem driver
typedef struct
{
    char *name; // Name of filesystem type

    // Mount operation
    // vfs_mount_t mount(char * device)
    vfs_mount_t (*mount)(char *);

} vfs_fs_type_t;

// Represents a mounted filesystem
typedef struct
{
    vfs_inode_t *root; // Root inode
    void *fs_state;    // Mounted filesystem private state

    // Destroy mountpoint
    // Also deallocates the vfs_mount_t object
    // void destroy(vfs_inode_t *inode)
    void (*unmount)(vfs_mount_t *);
} vfs_mount_t;
#pragma once

#include <stdint.h>
#include <stddef.h>;
#include <stdbool.h>

// Limits for filename and path
#define FILENAME_MAX 32
#define PATH_MAX 1024

// Mount point number
typedef uint32_t mount_point_t;

// VFS file handle
typedef uint32_t vfs_file_t;

#define VFS_FILE_NULL 0;

// File open options
typedef uint32_t fopts;

// File open options
#define FOPT_DIR (1 << 0);

// Inode type
typedef enum
{
    VFS_INTYPE_FILE,
    VFS_INTYPE_DIR,
} vfs_inode_type_t;

// Directory entry
typedef struct
{
    char name[FILENAME_MAX + 1]; // NULL terminated file name
    vfs_inode_type_t type;       // File or directory
                                 //
} vfs_dirent_t;

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
    //
    // List dirents children of inode
    // uint32_t readdir(vfs_inode_t *inode, uint32_t offset, uint32_t n, vfs_dirent_t *buf);
    // Returns the number of dirents read (if less than n, no more dirents to read)
    uint32_t (*readdir)(vfs_inode_t *, uint32_t, uint32_t, vfs_dirent_t *);

    // Lookup child in directory inode by name
    // vfs_inode_t *lookup(vfs_inode_t *inode, char *name)
    vfs_inode_t (*lookup)(vfs_inode_t *, char *);

    // Destroy inode
    // Deallocate inode and any private data
    // void destroy(vfs_inode_t *inode)
    void (*destroy)(vfs_inode_t *);
};

// Represents a mounted filesystem
// Is allocated by the FS driver
typedef struct _vfs_superblock_t vfs_superblock_t;
struct _vfs_superblock_t
{
    vfs_inode_t *root; // Root inode
    void *fs_state;    // Mounted filesystem private state

    // Destroy mountpoint
    // Also deallocates the vfs_superblock_t object
    // void destroy(vfs_inode_t *inode)
    void (*unmount)(vfs_superblock_t *);
};

// Filesystem type
// Represents a filesystem driver
typedef struct
{
    char *name; // Name of filesystem type

    // Mount operation
    // vfs_superblock_t mount(char *device, vfs_superblock_t **mount)
    bool (*mount)(char *, vfs_superblock_t **);

} vfs_fs_type_t;

// VFS file
typedef struct
{
    vfs_file_t handle;  // File handle
    vfs_inode_t *inode; // Associated inode
} vfs_file_slot_t;

/*
 * Initialize VFS
 */
void vfs_init();

//////// FS driver functions

/*
 * Register a new filesystem driver with the VFS
 */
bool vfs_register_fs_type(vfs_fs_type_t fs_type);

//////// Mountpoint handling functions

/*
 * Mount filesystem
 * #### Parameters
 *   - dev: device to mount (passed to the FS driver)
 *   - mp: mount point number
 *   - fs: file system type
 * #### Returns
 *   true on success
 */
bool vfs_mount(char *dev, mount_point_t mp, char *fs);

/*
 * Unmount filesystem
 * #### Parameters
 *   - mp: mount point number
 * #### Returns
 *   true on success
 */
bool vfs_unmount(mount_point_t mp);

//////// File handling functions

/*
 * Open filesystem file or directory
 * If FOPT_DIR is not passed, fails if it finds a directory,
 * else fail if it is a file.
 * #### Parameters
 *   - path: file path
 *   - opt: file open options
 * ####
 */
vfs_file_t vfs_open(char *path, fopts opt);
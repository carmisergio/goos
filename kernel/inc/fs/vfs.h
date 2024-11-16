#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Limits for filename and path
#define FILENAME_MAX 32
#define PATH_MAX 1024

// Mount point number
typedef uint32_t mount_point_t;

// VFS file handle
typedef int32_t vfs_file_handle_t;

// File open options
typedef uint32_t fopts;

// File open options
#define FOPT_DIR (1 << 0)   // Want directory from open()
#define FOPT_WRITE (1 << 1) // Want to be able to write to file

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
    uint32_t size;               // Size of file
} vfs_dirent_t;

// VFS inode
// Represents a file in the virtual file system
typedef struct _vfs_inode_t vfs_inode_t;
struct _vfs_inode_t
{
    char name[FILENAME_MAX + 1]; // Filename
    uint32_t size;               // File size
    vfs_inode_type_t type;       // Type of inode
    void *priv_data;             // Filesystem private data
    void *fs_state;              // Mountpoint private state

    // Reference to parent inode
    // vfs_inode_t *parent;

    // Unique identifiying information
    uint32_t id; // Unique identifier inside the mount point

    // Read data from inode
    // uint32_t read(vfs_inode_t *inode, uint8_t *buf, uint32_t offset, uint32_t length)
    int64_t (*read)(vfs_inode_t *, uint8_t *, uint32_t, uint32_t);

    // Write data to inode
    // uint32_t write(vfs_inode_t *inode, uint8_t *buf, uint32_t offset, uint32_t length
    int64_t (*write)(vfs_inode_t *, uint8_t *, uint32_t, uint32_t);
    //
    // List dirents children of inode
    // uint32_t readdir(vfs_inode_t *inode, vfs_dirent_t *buf, uint32_t offset, uint32_t n);
    // Returns the number of dirents read (if less than n, no more dirents to read)
    int64_t (*readdir)(vfs_inode_t *, vfs_dirent_t *, uint32_t, uint32_t);

    // Lookup child in directory inode by name
    // vfs_inode_t *lookup(vfs_inode_t *inode, vfs_inode_t**res, char *name)
    int32_t (*lookup)(vfs_inode_t *, vfs_inode_t **, char *);

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
    int32_t (*mount)(char *, vfs_superblock_t **);

} vfs_fs_type_t;

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
 *   0 on success, otherwise a negative value indicating the error
 */
int32_t vfs_mount(char *dev, mount_point_t mp, char *fs);

/*
 * Unmount filesystem
 * #### Parameters
 *   - mp: mount point number
 * #### Returns
 *   0 on success, otherwise a negative value indicating the error
 */
int32_t vfs_unmount(mount_point_t mp);

//////// File handling functions

/*
 * Open VFS file
 * If FOPT_DIR is not passed, fails if it finds a directory,
 * else fail if it is a file.
 * #### Parameters
 *   - path: file path
 *   - opt: file open options
 * #### Returns
 *    file handle (>= 0) on success, else error
 */
vfs_file_handle_t vfs_open(char *path, fopts opt);

/*
 * Close VFS file
 * #### Parameters
 *  - file: VFS file handle
 */
void vfs_close(vfs_file_handle_t file);

/*
 * Read entries from a directory
 * #### Parameters
 *  - file: VFS file handle of the directory
 *  - buf: buffer to place the directory entries
 *  - offset: offset from the start of the directory IN ENTRIES (not bytes)
 *  - n: max number of direcories to read IN ENTRIES (not bytes)
 * #### Returns
 *    number of directory entries read (>= 0) on success, else error
 *    If the number returned is < n, there are no more directories to read
 */
int64_t vfs_readdir(vfs_file_handle_t file, vfs_dirent_t *buf, uint32_t offset, uint32_t n);

/*
 * Read data from a file
 * #### Parameters
 *  - file: VFS file handle of the file
 *  - buf: buffer to read inot
 *  - offset: offset from the start of the file in bytes
 *  - n: max number of bytes to read
 * #### Returns
 *    number of bytes read (>= 0) on success, else error
 *    If the number returned is < n, there are no more bytes to read
 */
int64_t vfs_read(vfs_file_handle_t file, uint8_t *buf, uint32_t offset, uint32_t n);

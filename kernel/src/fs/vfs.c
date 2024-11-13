#include "fs/vfs.h"

#include "collections/dllist.h"
#include "string.h"

#include "mem/kalloc.h"
#include "fs/path.h"
#include "panic.h"

#define MAX_MOUNT_POINTS 16
#define MAX_FILES 32 // Number of simultaneously open files

// VFS file
typedef struct
{
    vfs_inode_t *inode; // Pointer to inode
    bool write;         // Is the file open for writing?
    mount_point_t mp;   // Mountpoint to which this file

    // Number of references to this file. If the references drop to 0, the inode
    // contained in the file is deallocated
    uint32_t ref_count;
} vfs_file_t;

// Internal functions
static vfs_fs_type_t *find_fs_type(const char *name);
static bool is_filesystem_busy(mount_point_t mp);
static vfs_file_handle_t find_free_file_slot();
static int32_t find_file_by_inode_id(mount_point_t mp, uint32_t id);
static bool compare_inodes(vfs_inode_t *a, vfs_inode_t *b);
static int32_t lookup_path(vfs_inode_t **res, vfs_inode_t *root, char *path);
static void superblock_unmount(vfs_superblock_t *sb);
static int32_t inode_lookup(vfs_inode_t *inode, vfs_inode_t **res, char *file_name);
static void inode_destroy(vfs_inode_t *inode);
static int64_t inode_readdir(vfs_inode_t *inode, vfs_dirent_t *buf,
                             uint32_t offset, uint32_t n);
static int64_t inode_read(vfs_inode_t *inode, uint8_t *buf,
                          uint32_t offset, uint32_t n);

// Global objects
dllist_t fs_types;
vfs_superblock_t *mount_points[MAX_MOUNT_POINTS];
vfs_file_t open_files[MAX_FILES];
vfs_file_handle_t next_vfs_file_handle;

void vfs_init()
{
    // Initialize mountpoint list
    for (size_t i = 0; i < MAX_MOUNT_POINTS; i++)
        mount_points[i] = NULL;

    // Initialize fs types list
    dllist_init(&fs_types);

    // Initialize open files list
    for (size_t i = 0; i < MAX_FILES; i++)
        open_files[i].ref_count = 0;

    // VFS file handles start from 0
    next_vfs_file_handle = 0;
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

int32_t vfs_mount(char *dev, mount_point_t mp, char *fs)
{
    // Check if mountpoint is valid and not already mounted
    if (mp >= MAX_MOUNT_POINTS || mount_points[mp] != NULL)
        return VFS_ENOMP;

    // Find correct FS driver
    vfs_fs_type_t *fs_type = find_fs_type(fs);
    if (!fs_type)
        return VFS_ENOFS;

    // Mount
    vfs_superblock_t *mount;
    int32_t res = fs_type->mount(dev, &mount);
    if (res < 0)
        return res;

    // Set mount point
    mount_points[mp] = mount;

    return 0;
}

int32_t vfs_unmount(mount_point_t mp)
{
    // Check if mountpoint is valid and mounted
    if (mp >= MAX_MOUNT_POINTS || mount_points[mp] == NULL)
        return VFS_ENOMP;

    // Check if there are open inodes which belong to this filesystem
    if (is_filesystem_busy(mp))
        return VFS_EBUSY;

    // Unmount
    superblock_unmount(mount_points[mp]);

    // Free mount slot
    mount_points[mp] = NULL;

    return 0;
}

vfs_file_handle_t vfs_open(char *path, fopts opt)
{
    uint32_t ret;

    // Parse mountpoint
    mount_point_t mp;
    if (!parse_path_mountpoint(&mp, &path))
        // No mountpoint in path
        return VFS_ENOENT;

    // Get root inode of the filesystem mounted at the
    // specified mountpoint
    if (mp >= MAX_MOUNT_POINTS || mount_points[mp] == NULL)
        // Invalid mountpoint
        return VFS_ENOENT;

    // Find inode for this path
    vfs_inode_t *inode;
    int32_t res = lookup_path(&inode, mount_points[mp]->root, path);
    if (res < 0)
        return res;

    // Check if file is of the desired type
    if (opt & FOPT_DIR)
    {
        // Want a directory
        if (inode->type == VFS_INTYPE_FILE)
        {
            ret = VFS_EWRONGTYPE;
            goto fail;
        }
    }
    else
    {
        // Want a file
        if (inode->type == VFS_INTYPE_DIR)
        {
            ret = VFS_EWRONGTYPE;
            goto fail;
        }
    }

    // Do we want to be able to write?
    bool write = (opt & FOPT_WRITE) != 0;

    // Check if there is a file with the same inode
    int32_t file_handle = find_file_by_inode_id(mp, inode->id);
    if (file_handle >= 0)
    {
        // Use file we found
        vfs_file_t *file = &open_files[file_handle];

        // Destroy this new copy of the inode, since we have the one associated
        // with this file
        if (inode != mount_points[mp]->root)
            inode_destroy(inode);

        // Cannot open for writing a file that's already open
        // Cannot open a file that's already open for writing
        if (write || file->write)
            return VFS_EBUSY;

        // Increment reference count
        file->ref_count++;

        return file_handle;
    }

    // Find a free file handle
    file_handle = find_free_file_slot();
    if (file_handle < 0)
    {
        // No free handles
        ret = VFS_ETOOMANY;
        goto fail;
    }

    // Use file we found
    vfs_file_t *file = &open_files[file_handle];

    // Set file parameters
    file->inode = inode;
    file->ref_count = 1;
    file->write = write;

    return file_handle;

fail:
    // Destroy new inode if it's not the root inode of the mount point
    if (inode != mount_points[mp]->root)
        inode_destroy(inode);
    return ret;
}

void vfs_close(vfs_file_handle_t file)
{
    // Check if file is valid and open
    if (file >= MAX_FILES || open_files[file].ref_count == 0)
        return;

    // Decrement reference count
    open_files[file].ref_count--;

    // When refrencs hit 0, destroy inode
    if (open_files[file].ref_count == 0)
    {
        if (open_files[file].inode != mount_points[open_files[file].mp]->root)
            inode_destroy(open_files[file].inode);
    }
}

int64_t vfs_readdir(vfs_file_handle_t file, vfs_dirent_t *buf, uint32_t offset, uint32_t n)
{
    // Check if file is valid and open
    if (file >= MAX_FILES || open_files[file].ref_count == 0)
        return VFS_ENOENT;

    // Get inode from file
    vfs_inode_t *inode = open_files[file].inode;

    // Perform read
    return inode_readdir(inode, buf, offset, n);
}

int64_t vfs_read(vfs_file_handle_t file, uint8_t *buf, uint32_t offset, uint32_t n)
{
    // Check if file is valid and open
    if (file >= MAX_FILES || open_files[file].ref_count == 0)
        return VFS_ENOENT;

    // Get inode from file
    vfs_inode_t *inode = open_files[file].inode;

    // Perform read
    return inode_read(inode, buf, offset, n);
}

/* Internal functions */
static vfs_fs_type_t *find_fs_type(const char *name)
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

// Check if a filesystem has open inodes in the open inodes list
static bool is_filesystem_busy(mount_point_t mp)
{
    // Iterate over open files list
    for (size_t i = 0; i < MAX_FILES; i++)
    {
        if (open_files[i].ref_count > 0 &&
            open_files[i].mp == mp)
            return true;
    }

    return false;
}

// Find a free slot in the open_files list
// Returns -1 on failure
static int32_t find_free_file_slot()
{
    // Iterate over open files list
    for (size_t i = 0; i < MAX_FILES; i++)
    {
        if (open_files[i].ref_count == 0)
            return i;
    }

    return -1;
}

// Find file in open files list based on the inode
// Returns -1 on failure
static int32_t find_file_by_inode_id(mount_point_t mp, uint32_t id)
{
    // Iterate over open files list
    for (size_t i = 0; i < MAX_FILES; i++)
    {
        if (open_files[i].ref_count > 0)
        {
            // Same mountpoint and same id => same file
            if (open_files[i].mp == mp && open_files[i].inode->id == id)
                return i;
        }
    }

    return -1;
}

// Find inode for a certain absolute path
// Returns 0 on success
static int32_t lookup_path(vfs_inode_t **res, vfs_inode_t *root, char *path)
{
    vfs_inode_t *cur_inode = root;

    // Follow path
    char file_name[FILENAME_MAX];
    while (parse_path_filename(file_name, &path))
    {
        // Look up child inode
        vfs_inode_t *child;
        int32_t res;
        if ((res = inode_lookup(cur_inode, &child, file_name)) < 0)
            return res;

        // Destroy current inode (if it's not the root inode)
        if (cur_inode != root)
            inode_destroy(cur_inode);

        cur_inode = child;
    }

    // Return result
    *res = cur_inode;

    return 0;
}

static void superblock_unmount(vfs_superblock_t *sb)
{
    if (!sb->unmount)
        panic("VFS_SUPERBLOCK_NOUNMOUNT", "Superblock doesn't implement unmount()");

    sb->unmount(sb);
}

static int32_t inode_lookup(vfs_inode_t *inode, vfs_inode_t **res, char *file_name)
{
    if (!inode->lookup)
        return VFS_ENOIMPL;

    return inode->lookup(inode, res, file_name);
}

static void inode_destroy(vfs_inode_t *inode)
{
    if (!inode->destroy)
        panic("VFS_INODE_NODESTROY", "Inode doesn't implement destroy()");

    inode->destroy(inode);
}

// N is the number of directory entries which fit in the buffer
static int64_t inode_readdir(vfs_inode_t *inode, vfs_dirent_t *buf,
                             uint32_t offset, uint32_t n)
{
    if (!inode->readdir)
        return VFS_ENOIMPL;

    return inode->readdir(inode, buf, offset, n);
}

static int64_t inode_read(vfs_inode_t *inode, uint8_t *buf,
                          uint32_t offset, uint32_t n)
{
    if (!inode->read)
        return VFS_ENOIMPL;

    return inode->read(inode, buf, offset, n);
}

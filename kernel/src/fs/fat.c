#include "fs/fat.h"

#include <string.h>

#include "fs/vfs.h"
#include "log.h"
#include "blkdev/blkdev.h"
#include "mem/kalloc.h"
#include "panic.h"
#include "console/console.h"
#include "error.h"

// #define DEBUG

// BIOS parameter block structure
typedef struct __attribute__((packed))
{
    uint16_t _res0;
    uint8_t _res1;
    char oem_ident[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t n_fats;
    uint16_t root_entries;
    uint16_t n_sectors; // If 0, see Large Sector Count
    uint8_t media_desc;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t n_heads;
    uint32_t hidden_sectors;
    uint32_t large_sector_count;

    // FAT12 EBPB
    uint8_t disk_n;
    uint8_t _res2;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t system_ident[8];
} bpb_t;

// FAT directory entry
typedef struct __attribute__((packed))
{

    union
    {
        struct __attribute__((packed))
        {
            uint8_t s_name[8];
            uint8_t s_ext[3];
        };
        struct __attribute__((packed))
        {
            uint8_t l_order;
            uint16_t l_name1[5];
        };
    };

    uint8_t attrs;

    union
    {
        struct __attribute__((packed))
        {
            uint8_t _s_res0;
            uint8_t s_creation_time_fine;
            uint16_t s_creation_time;
            uint16_t s_creation_date;
            uint16_t s_last_accessed_date;
            uint16_t s_fat_entry_high;
            uint16_t s_last_modified_time;
            uint16_t s_last_modified_date;
            uint16_t s_fat_entry_low;
            uint32_t s_size;
        };
        struct __attribute__((packed))
        {
            uint8_t l_type;
            uint8_t l_checksum;
            uint16_t l_name2[6];
            uint16_t _l_zero0;
            uint16_t l_name3[2];
        };
    };
} fat_dir_entry_t;

#define ATTR_RO 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLID 0x08
#define ATTR_DIR 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LFN (ATTR_RO | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLID)

// FAT filesystem private state
typedef struct
{
    blkdev_handle_t dev_handle;
    bpb_t bpb;

    // Did the underlying block device signal a media change?
    bool media_changed;

    // Buffers
    uint8_t *fat_cache; // FAT cache
    uint8_t *io_buf;    // I/O buffer

    // Useful information
    uint32_t data_start; // Data starting sector
                         // Used for computing the sectors for a cluster
} fs_state_t;

// FAT filesystem inode private data
typedef struct
{
    uint32_t *sector_list; // List of sectors
} inode_private_t;

// Internal functions
static int32_t fs_type_mount(const char *dev, vfs_superblock_t **mount);
static bool read_bpb(fs_state_t *fs_state);
static bool read_fat_cache(fs_state_t *fs_state);
static void superblock_unmount(vfs_superblock_t *mount);
static void inode_destroy(vfs_inode_t *inode);
static bool check_fat_magically(bpb_t *bpb);
static void destroy_fs_state(fs_state_t *state);
static vfs_inode_t *get_root_inode(fs_state_t *fs_state);
static int64_t inode_readdir(vfs_inode_t *inode, dirent_t *buf,
                             uint32_t offset, uint32_t n);
static int32_t inode_lookup(vfs_inode_t *inode, vfs_inode_t **res, const char *name);
static int64_t inode_read(vfs_inode_t *inode, uint8_t *buf,
                          uint32_t offset, uint32_t n);
static void direntry_name_from_short(char *name, fat_dir_entry_t *entry);
static inline uint32_t nblocks(uint32_t size);
static void free_inode_pdata(inode_private_t *pdata);
static uint32_t follow_sector_chain(uint32_t *sector_list,
                                    fs_state_t *fs_state, uint32_t start_cluster);
static bool read_fat_entry(uint32_t *entry, fs_state_t *fs_state, uint32_t cluster);
static bool check_media_changed(fs_state_t *fs_state);
static uint32_t cluster_start_sector(fs_state_t *fs_state, uint32_t cluster);
static void debug_sector_list_chain(uint32_t *sectors, uint32_t n);

void fat_init()
{
    vfs_fs_type_t fat = {
        .name = "fat",
        .mount = fs_type_mount,
    };

    // Register FAT filesystem driver
    if (!vfs_register_fs_type(fat))
        kprintf("[FAT] Unable to register fs type\n");
}

/* Internal functions */

static int32_t fs_type_mount(const char *dev, vfs_superblock_t **superblock)
{
    kprintf("[FAT] Mounting device %s\n", dev);

    // Get block device handle
    blkdev_handle_t dev_handle = blkdev_get_handle(dev);
    if (dev_handle == BLKDEV_HANDLE_NULL)
    {
        kprintf("[FAT] Unable to get handle for device %s\n", dev);
        return E_NOENT;
    }

    // Allocate filesytem state
    fs_state_t *fs_state = kalloc(sizeof(fs_state_t));
    if (!fs_state)
        goto fail_nostate;

    // Initialize state
    fs_state->dev_handle = dev_handle;
    fs_state->fat_cache = NULL;
    fs_state->io_buf = NULL;
    fs_state->media_changed = false;

    // Allocate read buffer
    if (!(fs_state->io_buf = kalloc(BLOCK_SIZE)))
        goto fail;

    // Read BIOS parameter block
    if (!read_bpb(fs_state))
        goto fail;

    // Check if filesystem is likely enough FAT
    if (!check_fat_magically(&fs_state->bpb))
        goto fail;

    // Fill FAT cache
    if (!read_fat_cache(fs_state))
        goto fail;

    // Get root directory inode
    vfs_inode_t *root = get_root_inode(fs_state);
    if (!root)
        goto fail;

    // Allocate VFS superblock structure
    vfs_superblock_t *sb = kalloc(sizeof(vfs_superblock_t));
    if (!sb)
        goto fail;

    // Construct superblock
    sb->fs_state = fs_state;
    sb->root = root;
    sb->unmount = superblock_unmount;

    // Copy superblock structure pointer to caller
    *superblock = sb;

    // Success!
    return 0;

fail:
    destroy_fs_state(fs_state);
fail_nostate:
    return E_UNKNOWN;
}

static void superblock_unmount(vfs_superblock_t *superblock)
{
    fs_state_t *state = superblock->fs_state;

    // Release block device handle
    blkdev_release_handle(state->dev_handle);

    // Free state structure
    destroy_fs_state(state);

    // Destroy root inode
    inode_destroy(superblock->root);

    // Free superblock
    kfree(superblock);
}

// Read Bios Parameter Block values into the filesystem state
static bool read_bpb(fs_state_t *fs_state)
{

    // Read superblock
    if (!blkdev_read(fs_state->io_buf, fs_state->dev_handle, 0))
        return false;

    // Copy values into state
    bpb_t *bpb = (bpb_t *)fs_state->io_buf;
    fs_state->bpb = *bpb;

#ifdef DEBUG
    kprintf("[FAT] Superblock information\n");
    kprintf("  Reserved sectors: %d\n", fs_state->bpb.reserved_sectors);
    kprintf("  Number of FATs: %d\n", fs_state->bpb.n_fats);
#endif

    return true;
}

// Read FAT into the filesystem state
static bool read_fat_cache(fs_state_t *fs_state)
{
    // Find FAT starting sector
    uint32_t fat_start = fs_state->bpb.reserved_sectors;

    // Allocate FAT cache
    if (!(fs_state->fat_cache = kalloc(fs_state->bpb.sectors_per_fat * BLOCK_SIZE)))
        return false;

    // Read FAT into memory
    if (!blkdev_read_n(fs_state->fat_cache, fs_state->dev_handle, fat_start,
                       fs_state->bpb.sectors_per_fat))
    {
        kfree(fs_state->fat_cache);
        return false;
    }

    return true;
}

// Destroy inode, freeing all memory (inode as well)
static void inode_destroy(vfs_inode_t *inode)
{
    // Free private data
    inode_private_t *prdata = inode->priv_data;
    kfree(prdata->sector_list);

    // Free inode fields
    kfree(inode->priv_data);

    // kprintf("Inode destroy!\n");

    // Free inode
    kfree(inode);
}

// Guesses if the volume is FAT or not
// Would it have been so hard to provide a magic number?
// Some kind of checksum? HELLO?!?!?!
static bool check_fat_magically(bpb_t *bpb)
{
    if (bpb->n_fats > 10)
        return false;

    if (bpb->bytes_per_sector != 512)
        return false;

    if (bpb->signature != 0x28 && bpb->signature != 0x29)
        return false;

    return true;
}

static void destroy_fs_state(fs_state_t *state)
{
    // Free FAT cache
    if (state->fat_cache)
        kfree(state->fat_cache);

    // Free IO buffer
    if (state->io_buf)
        kfree(state->io_buf);

    // Free fs state object
    kfree(state);
}

// Construct root directory inode
static vfs_inode_t *get_root_inode(fs_state_t *fs_state)
{
    // Compute sector information
    uint32_t start_sec = fs_state->bpb.reserved_sectors +
                         (fs_state->bpb.n_fats *
                          fs_state->bpb.sectors_per_fat);

    uint32_t n_sectors = (fs_state->bpb.root_entries *
                              sizeof(fat_dir_entry_t) +
                          BLOCK_SIZE - 1) /
                         BLOCK_SIZE;

    // Set data start in fs state
    fs_state->data_start = start_sec + n_sectors;

    // Allocate space for the sector list
    uint32_t *sector_list = kalloc(sizeof(uint32_t) * n_sectors);
    if (!sector_list)
        goto fail_nomem_sectlist;

    // Fill sector list
    for (uint32_t i = 0; i < n_sectors; i++)
        sector_list[i] = start_sec + i;

    // Allocate space for root private data
    inode_private_t *pdata = kalloc(sizeof(inode_private_t));
    if (!pdata)
        goto fail_nomem_pdata;

    // Initialize private data
    pdata->sector_list = sector_list;

    // Allocate VFS inode structure
    vfs_inode_t *inode = kalloc(sizeof(vfs_inode_t));
    if (!inode)
        goto fail_nomem_inode;

    // Fill inode fields
    inode->name[0] = '\0';
    inode->size = n_sectors * BLOCK_SIZE;
    inode->type = VFS_INTYPE_DIR;
    inode->priv_data = pdata;
    inode->fs_state = fs_state;
    inode->id = 0; // Cluster 0 is reserved, use it for the root dir
    inode->read = NULL;
    inode->write = NULL;
    inode->readdir = inode_readdir;
    inode->lookup = inode_lookup;
    inode->destroy = inode_destroy;

    return inode;

fail_nomem_inode:
    kfree(pdata);
fail_nomem_pdata:
    kfree(sector_list);
fail_nomem_sectlist:
    return NULL;
}

//// Inode functions
static int64_t inode_readdir(vfs_inode_t *inode, dirent_t *buf,
                             uint32_t offset, uint32_t n)
{
    fs_state_t *fs_state = inode->fs_state;
    inode_private_t *pdata = inode->priv_data;

    // Handle media change
    if (check_media_changed(fs_state))
        return E_MDCHNG;

    // Find out number of directory sectors
    uint32_t n_sectors = inode->size / BLOCK_SIZE;

    // Read directory sector by sector
    uint32_t dirs_read = 0, dirs_skipped = 0;
    bool more_dirs = true;
    bool has_lfn = false; // Do we have a long filename in the buffer?
    for (size_t i = 0; (i < n_sectors) && more_dirs && (dirs_read < n); i++)
    {
        uint32_t block = pdata->sector_list[i];

        // Read sector into IO guuffer
        if (!blkdev_read(fs_state->io_buf, fs_state->dev_handle, block))
            return E_IOERR;

        // Read all directory entries in the sector
        // for (fat_dir_entry_t *entry = (fat_dir_entry_t *)fs_state->io_buf;
        //      entry < (fat_dir_entry_t *)fs_state->io_buf + BLOCK_SIZE &&
        //      dirs_read < n;
        //      entry++)
        for (size_t j = 0; j < BLOCK_SIZE / sizeof(fat_dir_entry_t); j++)
        {
            fat_dir_entry_t *entry = &((fat_dir_entry_t *)fs_state->io_buf)[j];

            // If first byte of entry is 0, there are no more dirctories
            if (entry->s_name[0] == 0x00)
            {
                more_dirs = false;
                break;
            }

            // If first byte of entry is 0xE5, the entry is unused
            // Ignore it
            if (entry->s_name[0] == 0xE5)
                continue;

            // Check if this is a long filename entry
            if (entry->attrs == ATTR_LFN)
            {
                // direntry_process_lfn(buf[dirs_read].name, entry);
                // has_lfn = true;
                // TODO: add LFN support
                continue;
            }

            // Ignore Volume ID entries
            if (entry->attrs & ATTR_VOLID)
                continue;

            // Fill directory entry
            if (!has_lfn)
                direntry_name_from_short(buf[dirs_read].name, entry);

            // Ignore metadirectories '.' and '..'
            if (strcmp(buf[dirs_read].name, "..") == 0 ||
                strcmp(buf[dirs_read].name, ".") == 0)
                continue;

            if (dirs_skipped == offset)
            {
                buf[dirs_read].size = entry->s_size;
                buf[dirs_read].type = (entry->attrs & ATTR_DIR) != 0;
                dirs_read++;
            }
            else
            {
                dirs_skipped++;
            }

            has_lfn = false;
        }
    }

    return dirs_read;
}

static int32_t inode_lookup(vfs_inode_t *inode, vfs_inode_t **res, const char *name)
{
    fs_state_t *fs_state = inode->fs_state;
    inode_private_t *pdata = inode->priv_data;

    // Find out number of directory sectors
    uint32_t n_sectors = inode->size / BLOCK_SIZE;

    // Handle media change
    if (check_media_changed(fs_state))
        return E_MDCHNG;

    // Read directory sector by sector
    uint32_t dirs_read = 0;
    bool more_dirs = true;
    // bool has_lfn = false; // Do we have a long filename in the buffer?
    char name_buf[FILENAME_MAX];
    for (size_t i = 0; i < n_sectors && more_dirs; i++)
    {
        uint32_t block = pdata->sector_list[i];

        // Read sector into IO guuffer
        if (!blkdev_read(fs_state->io_buf, fs_state->dev_handle, block))
            return E_IOERR;

        // Read all directory entries in the sector
        for (fat_dir_entry_t *entry = (fat_dir_entry_t *)fs_state->io_buf;
             entry < (fat_dir_entry_t *)fs_state->io_buf + BLOCK_SIZE; entry++)
        {

            // If first byte of entry is 0, there are no more dirctories
            if (entry->s_name[0] == 0x00)
            {
                more_dirs = false;
                break;
            }

            // If first byte of entry is 0xE5, the entry is unused
            // Ignore it
            if (entry->s_name[0] == 0xE5)
                continue;

            // Check if this is a long filename entry
            if (entry->attrs == ATTR_LFN)
            {
                // direntry_process_lfn(buf[dirs_read].name, entry);
                // has_lfn = true;
                // TODO: add LFN support
                continue;
            }

            // Ignore Volume ID entries
            if (entry->attrs & ATTR_VOLID)
                continue;

            // Get name of entry
            direntry_name_from_short(name_buf, entry);

            // Ignore metadirectories '.' and '..'
            if (strcmp(name_buf, "..") == 0 ||
                strcmp(name_buf, ".") == 0)
                continue;

            // Compare entry name with requested filename
            if (strcmp(name_buf, name) == 0)
            {
                // Found file!

                // Check if file is a directory
                bool is_dir = (entry->attrs & ATTR_DIR) != 0;

                // Compute sector list length
                uint32_t n = follow_sector_chain(NULL, fs_state, entry->s_fat_entry_low);

                // Sanity check number of sectors
                // Only check on files, not directories
                if (!is_dir && n != nblocks(entry->s_size))
                    return E_INCON;

                // Allocate inode private data
                inode_private_t *new_pdata = kalloc(sizeof(inode_private_t));
                if (!pdata)
                    return E_NOMEM;

                // Allocate and construct sector list
                uint32_t *sector_list = kalloc(sizeof(uint32_t) * n);
                if (!sector_list)
                {
                    free_inode_pdata(pdata);
                    return E_NOMEM;
                }
                follow_sector_chain(sector_list, fs_state, entry->s_fat_entry_low);
                new_pdata->sector_list = sector_list;

                // Construct inode
                vfs_inode_t *new_inode = kalloc(sizeof(vfs_inode_t));
                if (!new_inode)
                {
                    free_inode_pdata(pdata);
                    return E_NOMEM;
                }

                strcpy(new_inode->name, name_buf);
                // FAT directories don't have a size, round it up to the number
                // of sectors
                new_inode->size = is_dir ? BLOCK_SIZE * n : entry->s_size;
                new_inode->type = is_dir ? VFS_INTYPE_DIR : VFS_INTYPE_FILE;
                new_inode->priv_data = new_pdata;
                new_inode->fs_state = fs_state;
                new_inode->id = entry->s_fat_entry_low;
                new_inode->read = is_dir ? NULL : inode_read;
                new_inode->write = NULL;
                new_inode->readdir = is_dir ? inode_readdir : NULL;
                new_inode->lookup = is_dir ? inode_lookup : NULL;
                new_inode->destroy = inode_destroy;

                *res = new_inode;
                return 0;
            }

            // has_lfn = false;
        }

        return E_NOENT;
    }

    return dirs_read;
}

static int64_t inode_read(vfs_inode_t *inode, uint8_t *buf,
                          uint32_t offset, uint32_t n)
{
    fs_state_t *fs_state = inode->fs_state;
    inode_private_t *pdata = inode->priv_data;

    // Handle media change
    if (check_media_changed(fs_state))
        return E_MDCHNG;

    // Find out number of file blocks
    uint32_t n_blocks = nblocks(inode->size);

    // Compute start sector
    uint32_t start_block = offset / BLOCK_SIZE;

    uint32_t bytes_read = 0;

    for (uint32_t block = start_block; block < n_blocks && bytes_read < n;
         block++)
    {
        uint32_t sector = pdata->sector_list[block];

        // Read sector into IO guuffer
        if (!blkdev_read(fs_state->io_buf, fs_state->dev_handle, sector))
            return E_IOERR;

        // Copy bytes
        uint32_t int_offset = offset % BLOCK_SIZE;        // Offset inside the sector
        uint32_t bytes_to_copy = BLOCK_SIZE - int_offset; // How many bytes to copy
        if (bytes_to_copy > n - bytes_read)               // Clamp with buffer size
            bytes_to_copy = n - bytes_read;
        if (bytes_to_copy > inode->size - offset) // Clamp with file size
            bytes_to_copy = inode->size - offset;
        memcpy(buf + bytes_read, fs_state->io_buf + int_offset, bytes_to_copy);
        bytes_read += bytes_to_copy;
        offset += bytes_to_copy;
    }

    return bytes_read;
}

// Fill directory entry name from 8.3 directory entry
static void direntry_name_from_short(char *name, fat_dir_entry_t *entry)
{
    size_t n = 0;

    // Check if there is an extension
    bool has_ext = false;
    for (size_t i = 0; i < 3; i++)
    {
        if (entry->s_ext[i] != ' ')
            has_ext = true;
    }

    // Read name field
    for (size_t i = 0; i < 8; i++)
    {
        // Remove spaces (they are used as padding ?!?! Why not a
        // non-printable character?)
        if (entry->s_name[i] != ' ')
        {
            name[n] = entry->s_name[i];
            n++;
        }
    }

    if (has_ext)
    {
        // Add extemsion separator ('.')
        name[n] = '.';
        n++;
    }

    for (size_t i = 0; i < 3; i++)
    {
        // Remove spaces (they are used as padding ?!?! Why not a
        // non-printable character?)
        if (entry->s_ext[i] != ' ')
        {
            name[n] = entry->s_ext[i];
            n++;
        }
    }

    // Null-terminate name
    name[n] = 0;
}

static inline uint32_t nblocks(uint32_t size)
{
    return (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
}

static void free_inode_pdata(inode_private_t *pdata)
{
    if (pdata->sector_list)
        kfree(pdata->sector_list);

    kfree(pdata);
}

static uint32_t follow_sector_chain(uint32_t *sector_list,
                                    fs_state_t *fs_state,
                                    uint32_t start_cluster)
{
    uint32_t n = 0;

    uint32_t cluster = start_cluster;
    do
    {
        // Add current cluster to list
        uint32_t sector = cluster_start_sector(fs_state, cluster);
        for (uint32_t i = 0; i < fs_state->bpb.sectors_per_cluster; i++)
        {
            // If sector_list is NULL, count only
            if (sector_list)
                sector_list[n] = sector + i;
            n++;
        }

        // Follow cluster link
        if (!read_fat_entry(&cluster, fs_state, cluster))
            return n; // Entry out of range, stop reading

    } while (cluster < 0xFF7 && cluster != 0); // End of cluster chain

    return n;
}

// Isolate a single FAT entry from the FAT cache
static bool read_fat_entry(uint32_t *entry, fs_state_t *fs_state,
                           uint32_t cluster)
{

    uint32_t fat_offset = cluster + (cluster / 2);
    uint32_t fat_entry = *(uint16_t *)(fs_state->fat_cache + fat_offset);

    // Check entry in range
    if (cluster >= fs_state->bpb.sectors_per_fat * BLOCK_SIZE)
        return false;

    if (cluster % 2 == 0)
        *entry = fat_entry & 0xFFF;
    else
        *entry = fat_entry >> 4;

    return true;
}

// Check if the filesystem is in a valid state
// (The block device's media hasn't been changed)
static bool check_media_changed(fs_state_t *fs_state)
{
    // If filesystem is already in an invalid state, exit
    if (fs_state->media_changed)
        return true;

    // Check if underlying block device media change has been raised
    if (blkdev_media_changed(fs_state->dev_handle))
    {
        kprintf("[FAT] Media changed\n");
        fs_state->media_changed = true;
    }

    return fs_state->media_changed;
}

// Compute starting sector of a cluster
static uint32_t cluster_start_sector(fs_state_t *fs_state, uint32_t cluster)
{
    return fs_state->data_start +
           (cluster - 2) * fs_state->bpb.sectors_per_cluster; // Cluster offset
}

static void debug_sector_list_chain(uint32_t *sectors, uint32_t n)
{
    kprintf("Sector list: \n");
    for (size_t i = 0; i < n; i++)
    {
        kprintf("%u, ", sectors[i]);
    }
    kprintf("\n");
}
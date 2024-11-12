#include "fs/fat.h"

#include "fs/vfs.h"
#include "log.h"
#include "blkdev/blkdev.h"
#include "mem/kalloc.h"
#include "panic.h"

#define DEBUG

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
        struct
        {
            uint8_t s_name[8];
            uint8_t s_ext[3];
        };
        struct
        {
            uint8_t l_order;
            uint16_t l_name1[5];
        };
    };

    uint8_t attrs;

    union
    {
        struct
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
        struct
        {
            uint8_t l_type;
            uint8_t l_checksum;
            uint16_t l_name2[6];
            uint8_t _l_zero0;
            uint16_t l_name3[3];
        };
    };
} fat_dir_entry_t;

#define ATTR_RO 0x01;
#define ATTR_HIDDEN 0x02;
#define ATTR_SYSTEM 0x04;
#define ATTR_VOLID 0x08;
#define ATTR_DIR 0x10;
#define ATTR_ARCHIVE 0x20;
#define ATTR_LFN (ATTR_RO | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLID)

// FAT filesystem private state
typedef struct
{
    blkdev_handle_t dev_handle;
    bpb_t bpb;
    uint8_t *fat_cache; // Cached File Allocation Table

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
int32_t fs_type_mount(char *dev, vfs_superblock_t **mount);
bool read_bpb(fs_state_t *fs_state);
bool read_fat_cache(fs_state_t *fs_state);
void superblock_unmount(vfs_superblock_t *mount);
void inode_destroy(vfs_inode_t *inode);
bool check_fat_magically(bpb_t *bpb);
uint32_t total_sectors(bpb_t *bbp);
void destroy_fs_state(fs_state_t *state);
vfs_inode_t *get_root_inode(fs_state_t *fs_state);

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
int32_t fs_type_mount(char *dev, vfs_superblock_t **superblock)
{
    kprintf("[FAT] Mounting device %s\n", dev);

    // Get block device handle
    blkdev_handle_t dev_handle = blkdev_get_handle(dev);
    if (dev_handle == BLKDEV_HANDLE_NULL)
    {
        kprintf("[FAT] Unable to get handle for device %s\n", dev);
        return VFS_ENOENT;
    }

    // Allocate filesytem state
    fs_state_t *fs_state = kalloc(sizeof(fs_state_t));
    if (!fs_state)
        goto fail_nostate;

    // Initialize state
    fs_state->dev_handle = dev_handle;
    fs_state->fat_cache = NULL;

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
    return VFS_EUNKNOWN;
}

void superblock_unmount(vfs_superblock_t *superblock)
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
bool read_bpb(fs_state_t *fs_state)
{
    // Allocate read buffer
    uint8_t *buf = kalloc(BLOCK_SIZE);
    if (!buf)
        return false;

    // Read superblock
    if (!blkdev_read(buf, fs_state->dev_handle, 0))
        // Free superblock buffer
        kfree(buf);

    // Copy values into state
    bpb_t *bpb = (bpb_t *)buf;
    fs_state->bpb = *bpb;

#ifdef DEBUG
    kprintf("[FAT] Superblock information\n");
    kprintf("  Reserved sectors: %d\n", fs_state->bpb.reserved_sectors);
    kprintf("  Number of FATs: %d\n", fs_state->bpb.n_fats);
#endif

    // Free superblock buffer
    kfree(buf);

    return true;
}

// Read FAT into the filesystem state
bool read_fat_cache(fs_state_t *fs_state)
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
void inode_destroy(vfs_inode_t *inode)
{
    // Free private data
    inode_private_t *prdata = inode->priv_data;
    kfree(prdata->sector_list);

    // Free inode fields
    kfree(inode->priv_data);

    kprintf("Inode destroy!\n");

    // Free inode
    kfree(inode);
}

// Guesses if the volume is FAT or not
// Would it have been so hard to provide a magic number?
// Some kind of checksum? HELLO?!?!?!
bool check_fat_magically(bpb_t *bpb)
{
    if (bpb->n_fats > 10)
        return false;

    if (bpb->bytes_per_sector != 512)
        return false;

    if (bpb->signature != 0x28 && bpb->signature != 0x29)
        return false;

    return true;
}

uint32_t total_sectors(bpb_t *bbp)
{
    return bbp->n_sectors == 0 ? bbp->large_sector_count : bbp->n_sectors;
}

void destroy_fs_state(fs_state_t *state)
{
    // Free FAT cache
    if (state->fat_cache)
        kfree(state->fat_cache);

    kprintf("Destroy fs state\n");

    // Free fs state object
    kfree(state);
}

// Construct root directory inode
vfs_inode_t *get_root_inode(fs_state_t *fs_state)
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
    inode->readdir = NULL;
    inode->lookup = NULL;
    inode->destroy = inode_destroy;

    return inode;

fail_nomem_inode:
    kfree(pdata);
fail_nomem_pdata:
    kfree(sector_list);
fail_nomem_sectlist:
    return NULL;
}
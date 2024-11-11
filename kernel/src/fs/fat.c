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
    char volume_label[11];
    char system_ident[8];
} bpb_t;

// FAT directory entry
typedef struct __attribute__((packed))
{
    char name[8];
    char ext[3];
    uint8_t attrs;
    uint8_t _res0;
    uint8_t creation_time_fine;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_accessed_date;
    uint16_t fat_entry_high;
    uint16_t last_modified_time;
    uint16_t last_modified_date;
    uint16_t fat_entry_low;
    uint32_t size;
} fat_dir_entry_t;

// FAT filesystem private state
typedef struct
{
    blkdev_handle_t dev_handle;
    bpb_t bpb;
    uint8_t *fat_cache; // Cached File Allocation Table
} mount_state_t;

// FAT filesystem inode private data
typedef struct
{
    bool is_dir;
    uint32_t *block_list;
} inode_private_t;

// Internal functions
bool fs_type_mount(char *dev, vfs_superblock_t **mount);
void superblock_unmount(vfs_superblock_t *mount);
void inode_destroy(vfs_inode_t *inode);
bool check_fat_magically(bpb_t *bpb);

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
bool fs_type_mount(char *dev, vfs_superblock_t **superblock)
{
    kprintf("[FAT] Mounting device %s\n", dev);

    // Get block device handle
    blkdev_handle_t dev_handle = blkdev_get_handle(dev);
    if (dev_handle == BLKDEV_HANDLE_NULL)
    {
        kprintf("[FAT] Unable to get handle for device %s\n", dev);
        return false;
    }

    // Allocate mount point
    vfs_superblock_t *superblock_s = kalloc(sizeof(vfs_superblock_t));
    if (!superblock_s)
        goto fail_nomem_superblock;

    // Allocate filesytem state
    mount_state_t *fs_state = kalloc(sizeof(mount_state_t));
    if (!fs_state)
        goto fail_nomem_state;

    // Set up FS state
    fs_state->dev_handle = dev_handle;

    // Allocate read buffer
    uint8_t *buf = kalloc(BLOCK_SIZE);
    if (!buf)
        goto fail_nomem_buf;

    // Read superblock
    if (!blkdev_read(buf, fs_state->dev_handle, 0))
    {
        // Free superblock buffer
        kfree(buf);
        goto fail_superblock_read_fail;
    }

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

    // Check if filesystem is probably FAT
    if (!check_fat_magically(&fs_state->bpb))
        goto fail_not_fat;

    // Find FAT starting sector
    uint32_t fat_start = fs_state->bpb.reserved_sectors;

    // Allocate FAT cache
    if (!(fs_state->fat_cache = kalloc(fs_state->bpb.sectors_per_fat * BLOCK_SIZE)))
        goto fail_nomem_fat_cache;

    // Read FAT into memory
    if (!blkdev_read_n(fs_state->fat_cache, fs_state->dev_handle, fat_start,
                       fs_state->bpb.sectors_per_fat))
        goto fail_fat_read_fail;

    // Construct root directory inode
    vfs_inode_t *root = kalloc(sizeof(vfs_inode_t));
    if (!root)
        goto fail_nomem_root;

    uint32_t root_start = fat_start + (fs_state->bpb.n_fats * fs_state->bpb.sectors_per_fat);
    uint32_t root_blocks = (fs_state->bpb.root_entries * sizeof(fat_dir_entry_t)) / BLOCK_SIZE;
    uint32_t *root_blocklist = kalloc(sizeof(uint32_t) * root_blocks);
    if (!root_blocklist)
        goto fail_nomem_root_blocklist;
    for (uint32_t i = 0; i < root_blocks; i++)
        root_blocklist[i] = root_start + i;

    // Allocate space for root private data
    inode_private_t *pdata;
    if (!(pdata = kalloc(sizeof(inode_private_t))))
        goto fail_nomem_privdata;

    pdata->block_list = root_blocklist;

    root->name = kalloc(10);
    root->type = VFS_INTYPE_DIR;
    root->priv_data = pdata;
    root->fs_state = fs_state;
    root->read = NULL;
    root->write = NULL;
    root->lookup = NULL;
    root->destroy = inode_destroy;

    // Construct mount structure
    superblock_s->fs_state = fs_state;
    superblock_s->unmount = superblock_unmount;
    superblock_s->root = root;

    // Copy superblock structure pointer to caller
    *superblock = superblock_s;

    return true;
fail_nomem_root_blocklist:
    kfree(root->priv_data);
fail_nomem_privdata:
    kfree(root);
fail_nomem_root:
fail_fat_read_fail:
    kfree(fs_state->fat_cache);
fail_nomem_fat_cache:
fail_not_fat:
fail_superblock_read_fail:
fail_nomem_buf:
    kfree(fs_state);
fail_nomem_state:
    kfree(superblock_s);
fail_nomem_superblock:
    return false;
}

void superblock_unmount(vfs_superblock_t *superblock)
{
    mount_state_t *state = superblock->fs_state;

    // Release block device handle
    blkdev_release_handle(state->dev_handle);

    // Free state structure
    kfree(state);

    // Destroy root inode
    superblock->root->destroy(superblock->root);

    // Free superblock
    kfree(superblock);
}

// Destroy inode, freeing all memory (inode as well)
void inode_destroy(vfs_inode_t *inode)
{
    // Free private data
    inode_private_t *prdata = inode->priv_data;
    kfree(prdata->block_list);

    // Free inode fields
    kfree(inode->name);
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
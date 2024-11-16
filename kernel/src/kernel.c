#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "string.h"

#include "mini-printf.h"

#include "log.h"
#include "console/console.h"
#include "mem/mem.h"
#include "mem/physmem.h"
#include "mem/vmem.h"
#include "boot/boot_info.h"
#include "panic.h"
#include "boot/multiboot_structs.h"
#include "drivers/vga.h"
#include "drivers/serial.h"
#include "int/interrupts.h"
#include "drivers/pit.h"
#include "drivers/kbdctl.h"
#include "clock.h"
#include "kbd/kbd.h"
#include "sysreq.h"
#include "syscall/go_user.h"
#include "boot/boot.h"
#include "cpu.h"
#include "proc/proc.h"
#include "mem/kalloc.h"
#include "blkdev/blkdev.h"
#include "drivers/ramdisk.h"
#include "drivers/cmos.h"
#include "drivers/fdc.h"
#include "fs/vfs.h"
#include "fs/fat.h"
#include "fs/path.h"
#include "proc/elf.h"

#define SYSTEM_DISK_DEV "fd0"
#define SYSTEM_DISK_FS "fat"
#define INIT_PROC_BIN "0:/BIN/MINIMAL"

// Boot information structure
boot_info_t boot_info;

uint8_t userland_stack[4096] __attribute__((aligned(4)));

// void do_syscall()
// {
//     __asm__ volatile(
//         "int $0x30" : : "a"(0));
// }

// void call_syscall_delay_ms(uint32_t time)
// {
//     __asm__ volatile(
//         "int $0x30" : : "b"(time), "a"(0x0110));
// }

// void call_syscall_console_write(char *s, size_t n)
// {
//     __asm__ volatile(
//         "int $0x30" : : "a"(0x200), "b"(s), "c"(n));
// }

// void userland_main()
// {
//     char buf[50];

//     // Execute system call

//     while (true)
//     {
//         for (uint32_t i = 0; i < 500; i++)
//         {
//             mini_snprintf(buf, 50, "Hello from\x1b[91m userspace\x1b[0m! %d\n", i);
//             call_syscall_console_write(buf, strlen(buf));
//         }
//         call_syscall_delay_ms(1000);
//     }
// }

// void kalloc_test()
// {
//     // Allocate, use, and free some memory
//     void **memories = kalloc(sizeof(void *) * 500);

//     // kprintf("Got memory: 0x%x\n", mem_palloc_k(1));

//     for (int j = 0; j < 500; j++)
//     {
//         for (int i = 1; i < 100; i++)
//         {
//             memories[i] = kalloc(i * 20);
//             memset(memories[i], 0x12, i * 20);
//         }

//         for (int i = 1; i < 100; i++)
//         {
//             if (i % 2 != 0)
//                 kfree(memories[i]);
//         }

//         void *tmp = kalloc(100);
//         memset(tmp, 0x56, 100);
//         kfree(tmp);

//         kdbg("Freed!\n");
//         // kalloc_dbg_block_chain();
//         for (int i = 1; i < 100; i++)
//         {
//             if (i % 2 == 0)
//                 kfree(memories[i]);
//         }

//         kdbg("Freed!\n");
//     }

//     kprintf("OK!\n");
//     kalloc_dbg_block_chain();
// }

// void console_test()
// {
//     char *str = "\e[31mRED \e[32mGREEN \e[34mBLUE \e[0m\n";
//     console_write(str, strlen(str));

//     str = "\e[41mRED \e[42mGREEN \e[44mBLUE \e[0m\n";
//     console_write(str, strlen(str));

//     str = "\e[91;42mTEST\e[0m\n";
//     console_write(str, strlen(str));

//     while (true)
//     {
//         // Read name
//         console_write("Name: ", 6);
//         char buf[100];
//         int32_t n = console_readline(buf, 100);
//         buf[n] = '\0';
//         kprintf("\nHello, %s!\n", buf);
//         clock_delay_ms(500);
//     }

//     for (;;)
//     {
//         // uint32_t time = clock_get_local();
//         // kprintf("Current time: %02d:%02d:%02d\n", time / 3600, ((time / 60) % 60), time % 60);

//         clock_delay_ms(1000);
//     }
// }

// void test_ramdisk()
// {
//     size_t nblocks = 14 * 1024 * 1024 / 512; // 4M
//     ramdisk_create(0, nblocks);
//     blkdev_debug_devices();

//     blkdev_handle_t handle = blkdev_get_handle("rd0");
//     if (handle == BLKDEV_HANDLE_NULL)
//     {
//         kprintf("Error getting handle!\n");
//         return;
//     }

//     kprintf("Handle: %d\n", handle);

//     uint8_t *buf = kalloc(BLOCK_SIZE);

//     kprintf("Media changed: %d\n", blkdev_media_changed(handle));

//     // Write data
//     memset(buf, 0xFF, BLOCK_SIZE);
//     for (size_t i = 0; i < nblocks; i++)
//     {
//         if (!blkdev_write(buf, handle, i))
//         {
//             kprintf("Write fail!\n");
//             return;
//         }
//     }
//     memset(buf, 0x00, BLOCK_SIZE);

//     // Read data
//     if (!blkdev_read(buf, handle, 0))
//     {
//         kprintf("Read fail!\n");
//         return;
//     }

//     // Result
//     for (size_t i = 0; i < BLOCK_SIZE; i++)
//     {
//         kprintf("%x ", buf[i]);
//     }
//     kprintf("\n");

//     blkdev_release_handle(handle);
// }

// void test_floppy()
// {

//     blkdev_handle_t handle = blkdev_get_handle("fd0");
//     if (handle == BLKDEV_HANDLE_NULL)
//     {
//         kprintf("Error getting handle!\n");
//         return;
//     }

//     kprintf("Handle: %d\n", handle);

//     uint8_t *buf = kalloc(BLOCK_SIZE);

//     // Write data
//     memset(buf, 0xFF, BLOCK_SIZE);
//     // for (size_t i = 0; i < nblocks; i++)
//     // {
//     // if (!blkdev_write(buf, handle, 0))
//     // {
//     //     kprintf("Write fail!\n");
//     //     return;
//     // }
//     // }
//     // memset(buf, 0x00, BLOCK_SIZE);

//     // clock_delay_ms(5000);

//     while (true)
//     {
//         // Read data
//         for (int i = 0; i < 100; i++)
//         {
//             kprintf("Media changed: %d\n", blkdev_media_changed(handle));
//             if (!blkdev_read(buf, handle, i))
//             {
//                 kprintf("Read fail!\n");
//                 return;
//             }

//             // Result
//             for (size_t i = 0; i < BLOCK_SIZE; i++)
//             {
//                 kprintf("%x ", buf[i]);
//             }
//             kprintf("\n");
//         }

//         clock_delay_ms(5000);
//     }

//     blkdev_release_handle(handle);
// }

// void test_fs_mem_leaks()
// {
//     kalloc_dbg_block_chain();
//     // Mount
//     if (vfs_mount("fd0", 0, "fat") >= 0)
//         kprintf("Mount success!\n");
//     else
//         kprintf("Mount failure!\n");

//     vfs_file_handle_t file1, file2, file3;

//     for (int i = 0; i < 100; i++)
//     {
//         if ((file1 = vfs_open("0:/BOOT", FOPT_DIR)) < 0)
//             kprintf("Open error: %d\n", file1);

//         // Try to read directory
//         vfs_dirent_t dir_buf[1];
//         int64_t ndirs;
//         int32_t offst = 0;
//         do
//         {
//             if ((ndirs = vfs_readdir(file1, dir_buf, offst, 1)) < 0)
//             {
//                 kprintf("Error reading directory: %d\n", ndirs);
//                 break;
//             }

//             for (size_t i = 0; i < ndirs; i++)
//             {
//                 kprintf(" - %s\n", dir_buf[i].name);
//             }

//             offst += ndirs;

//         } while (ndirs >= 1);

//         if ((file2 = vfs_open("0:/BOOT/MEMTEST.BIN", 0)) < 0)
//             kprintf("Open error: %d\n", file2);

//         do
//         {
//             if ((ndirs = vfs_readdir(file2, dir_buf, offst, 1)) < 0)
//             {
//                 kprintf("Error reading directory: %d\n", ndirs);
//                 break;
//             }

//             for (size_t i = 0; i < ndirs; i++)
//             {
//                 kprintf(" - %s\n", dir_buf[i].name);
//             }

//             offst += ndirs;

//         } while (ndirs >= 1);

//         vfs_close(file1);

//         vfs_close(file2);
//     }

//     if (vfs_unmount(0) >= 0)
//         kprintf("Unmount success!\n");
//     else
//         kprintf("Unmount failure!\n");
//     kalloc_dbg_block_chain();
// }

// void test_fs_read()
// {
//     if (vfs_mount("fd0", 0, "fat") >= 0)
//         kprintf("Mount success!\n");
//     else
//         kprintf("Mount failure!\n");

//     vfs_file_handle_t file;
//     if ((file = vfs_open("0:/BIN/HELLO.TXT", 0)) < 0)
//     {
//         kprintf("Open error: %d\n", file);
//         goto noread;
//     }

//     while (true)
//     {
//         // Do read
//         uint32_t offset = 0;
//         uint8_t *file_buf = mem_palloc_k(1);
//         int64_t bytes_read;

//         do
//         {
//             if ((bytes_read = vfs_read(file, file_buf, offset, MEM_PAGE_SIZE)) < 0)
//             {
//                 kprintf("Error reading file: %d\n", bytes_read);
//                 break;
//             }

//             console_write(file_buf, bytes_read);
//             console_write("\n", 1);
//             console_write("\n", 1);

//             offset += bytes_read;

//         } while (bytes_read >= 1);

//         clock_delay_ms(1000);
//     }

//     vfs_close(file);

// noread:

//     kprintf("\n");

//     if (vfs_unmount(0) >= 0)
//         kprintf("Unmount success!\n");
//     else
//         kprintf("Unmount failure!\n");
//     kalloc_dbg_block_chain();
// }

// void test_vas_switching()
// {
//     kprintf("In original address space: \n");

//     pde_t *orig_vas = vmem_cur_vas();

//     uint32_t *kvar = kalloc(sizeof(uint32_t));

//     // vmem_log_pagedir();
//     // vmem_log_vaddrspc();

//     mem_make_avail((void *)0x10000, 1);
//     uint32_t *uvar = (uint32_t *)0x10000;

//     void *pp = physmem_alloc();
//     kprintf("Physmem: 0x%x\n", pp);
//     physmem_free(pp);

//     // vmem_log_pagedir();
//     // vmem_log_vaddrspc();

//     *kvar = 1234;
//     *uvar = 5678;
//     kprintf(" - KVAR: %u\n", *kvar);
//     kprintf(" - UVAR: %u\n", *uvar);

//     // Switch address space
//     pde_t *new_vas = vmem_new_vas();
//     if (!new_vas)
//     {
//         kprintf("Error creating new VAS\n");
//         return;
//     }
//     vmem_switch_vas(new_vas);

//     // vmem_log_pagedir();
//     // vmem_log_vaddrspc();

//     mem_make_avail((void *)0x10000, 1);

//     // vmem_log_pagedir();
//     // vmem_log_vaddrspc();
//     kprintf("In new address space: \n");

//     *kvar = 4321;
//     *uvar = 8765;
//     kprintf(" - KVAR: %u\n", *kvar);
//     kprintf(" - UVAR: %u\n", *uvar);

//     // Destroy new userspace
//     vmem_destroy_uvas();

//     // Go back to original address space
//     vmem_switch_vas(orig_vas);
//     vmem_delete_vas(new_vas);

//     kprintf("In original address space: \n");

//     kprintf(" - KVAR: %u\n", *kvar);
//     kprintf(" - UVAR: %u\n", *uvar);

//     void *ppp = physmem_alloc();
//     kprintf("Physmem: 0x%x\n", ppp);
//     physmem_free(ppp);
// }
//

void vfs_load_test()
{
    vfs_file_handle_t file;

    // Open file
    // if ((file = vfs_open("0:/BIN/MINIMAL", 0)) < 0)
    if ((file = vfs_open(INIT_PROC_BIN, 0)) < 0)
    {
        kprintf("Unable to open ELF file: %d\n", file);
        return;
    }

    // Load program
    int32_t res;
    void *entry;
    if ((res = elf_load(file, &entry)) >= 0)
    {
        kprintf("ELF load success!\n");
        // kprintf(" Entry point: 0x%x\n", entry);
    }
    else
    {
        kprintf("ELF load error: %d\n", res);
        vfs_close(file);
        return;
    }

    // Close file
    vfs_close(file);

    proc_setup_cpu_ctx(entry);

    // Transfer control to user program
    go_userspace(&proc_cur()->cpu_ctx);
}

void logging_init();
void subsystems_init();
void drivers_init();
void userspace_init();
/*
 * Main kernel entry point
 * Remember to initialize boot_info before calling this!
 */
void kmain(multiboot_info_t *mbd)
{
    // Initialize kernel logging
    logging_init();
    kprintf("\e[94mGOOS\e[0m starting...\n");

    // Initialize memory management
    mem_init(mbd);

    // Initialize interrupts
    interrupts_init();

    // Initialize subsystems
    subsystems_init();

    // Initialize drivers
    drivers_init();

    // Go to userspace
    userspace_init();

    // Should never reach here!
}

// Initialize kernel logging
void logging_init()
{
    console_init();
    kprintf_init();
}

// Initialize subsystems
void subsystems_init()
{
    kbd_init();
    console_init_kbd();
    blkdev_init();
    vfs_init();
}

// Initialize drivers
void drivers_init()
{
    clock_init();
    kbdctl_init();
    sysreq_init();
    fdc_init();
    fat_init();
}

// Initialize userspace
void userspace_init()
{
    // Mount system disk
    kprintf("Mounting system disk...\n");
    if (vfs_mount(SYSTEM_DISK_DEV, 0, SYSTEM_DISK_FS) < 0)
        panic("SYSTEM_DISK_MOUNT", "Unable to mount system disk!");

    // Initialize process managemnt
    proc_init();

    // Start handlingn processes
    vfs_load_test();
}
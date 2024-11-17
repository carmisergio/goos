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
#include "error.h"
#include "drivers/isadma.h"

const char *SYSTEM_DISK_DEV = "fd0";
const char *SYSTEM_DISK_FS = "fat";
const char *INIT_BIN = "0:/BIN/MINIMAL";

// Global Boot information structure
boot_info_t boot_info;

void logging_init();
void subsystems_init();
void drivers_init();
bool userspace_init();
int32_t start_init_proc();

void test_canonicize()
{
    char res[PATH_MAX + 1];
    char canon[PATH_MAX + 1];
    char buf[PATH_MAX + 1];
    while (true)
    {
        kprintf("> ");
        uint32_t n = console_readline(buf, PATH_MAX);
        buf[n] = 0;

        if (!path_canonicalize(canon, buf))
        {
            kprintf("ERR!\n");
            continue;
        }

        kprintf("* ");
        n = console_readline(buf, PATH_MAX);
        buf[n] = 0;

        if (path_resolve_relative(res, canon, buf))
            kprintf("Result: %s\n", res);
        else
            kprintf("Err!\n");
    }
}

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

    // Only reaches here when initialization failed
    while (true)
        ;
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
    isadma_init();
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
bool userspace_init()
{
    int32_t res;

    // Mount system disk
    kprintf("[INIT] Mounting system disk (%s)\n", SYSTEM_DISK_DEV);
    if ((res = vfs_mount(SYSTEM_DISK_DEV, 0, SYSTEM_DISK_FS)) < 0)
    {
        kprintf("[INIT] Unable to mount system disk: %s\n", error_get_message(res));
        return false;
    }

    // Initialize process managemnt
    proc_init();

    // Start init process
    if ((res = start_init_proc() < 0))
    {
        kprintf("[INIT] Unable to start init process: %s\n", error_get_message(res));
        return false;
    }

    // Should never reach here
    return true;
}

int32_t start_init_proc()
{
    vfs_file_handle_t file;

    // Open executable file
    if ((file = vfs_open(INIT_BIN, 0)) < 0)
    {
        return file;
    }

    // Load program
    int32_t res;
    void *entry;
    if ((res = elf_load(file, &entry)) < 0)
        goto fail;

    // Close file
    vfs_close(file);

    // Don't print kernel debug information on the console
    // from now on
    kprintf_suppress_console(true);

    // Set up CPU context for process execution
    proc_setup_cpu_ctx(entry);

    // Transfer control to user program
    go_userspace(&proc_cur()->cpu_ctx);

    // Should never reach here
    return E_UNKNOWN;

fail:
    vfs_close(file);
    return res;
}
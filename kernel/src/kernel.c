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

// Boot information structure
boot_info_t boot_info;

uint8_t userland_stack[4096] __attribute__((aligned(4)));

void do_syscall()
{
    __asm__ volatile(
        "int $0x30" : : "a"(0));
}

void call_syscall_delay_ms(uint32_t time)
{
    __asm__ volatile(
        "int $0x30" : : "b"(time), "a"(0x0110));
}

void call_syscall_console_write(char *s, size_t n)
{
    __asm__ volatile(
        "int $0x30" : : "a"(0x200), "b"(s), "c"(n));
}

void userland_main()
{
    char buf[50];

    // Execute system call

    while (true)
    {
        for (uint32_t i = 0; i < 500; i++)
        {
            mini_snprintf(buf, 50, "Hello from\x1b[91m userspace\x1b[0m! %d\n", i);
            call_syscall_console_write(buf, strlen(buf));
        }
        call_syscall_delay_ms(1000);
    }
}

void kalloc_test()
{
    // Allocate, use, and free some memory
    void **memories = kalloc(sizeof(void *) * 500);

    // kprintf("Got memory: 0x%x\n", mem_palloc_k(1));

    for (int j = 0; j < 500; j++)
    {
        for (int i = 1; i < 100; i++)
        {
            memories[i] = kalloc(i * 20);
            memset(memories[i], 0x12, i * 20);
        }

        for (int i = 1; i < 100; i++)
        {
            if (i % 2 != 0)
                kfree(memories[i]);
        }

        void *tmp = kalloc(100);
        memset(tmp, 0x56, 100);
        kfree(tmp);

        kdbg("Freed!\n");
        // kalloc_dbg_block_chain();
        for (int i = 1; i < 100; i++)
        {
            if (i % 2 == 0)
                kfree(memories[i]);
        }

        kdbg("Freed!\n");
    }

    kprintf("OK!\n");
    kalloc_dbg_block_chain();
}

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

void test_ramdisk()
{
    size_t nblocks = 14 * 1024 * 1024 / 512; // 4M
    ramdisk_create(0, nblocks);
    blkdev_debug_devices();

    blkdev_handle_t handle = blkdev_get_handle("rd0");
    if (handle == BLKDEV_HANDLE_NULL)
    {
        kprintf("Error getting handle!\n");
        return;
    }

    kprintf("Handle: %d\n", handle);

    uint8_t *buf = kalloc(BLOCK_SIZE);

    kprintf("Media changed: %d\n", blkdev_media_changed(handle));

    // Write data
    memset(buf, 0xFF, BLOCK_SIZE);
    for (size_t i = 0; i < nblocks; i++)
    {
        if (!blkdev_write(buf, handle, i))
        {
            kprintf("Write fail!\n");
            return;
        }
    }
    memset(buf, 0x00, BLOCK_SIZE);

    // Read data
    if (!blkdev_read(buf, handle, 0))
    {
        kprintf("Read fail!\n");
        return;
    }

    // Result
    for (size_t i = 0; i < BLOCK_SIZE; i++)
    {
        kprintf("%x ", buf[i]);
    }
    kprintf("\n");

    blkdev_release_handle(handle);
}

/*
 * Main kernel entry point
 * Remember to initialize boot_info before calling this!
 */
void kmain(multiboot_info_t *mbd)
{
    // Initialize kernel logging
    console_init();
    kprintf_init();
    kprintf("\e[94mGOOS\e[0m starting...\n");

    // Initialize memory management
    mem_init(mbd);

    // Initialize interrupts
    interrupts_init();

    // Initialize subsystems
    kbd_init();
    console_init_kbd();
    blkdev_init();

    // Initialize drivers
    clock_init();
    kbdctl_init();
    sysreq_init();

    kprintf("BOOTED!\n");

    test_ramdisk();

    // proc_ctx_t proc_ctx = {
    //     .eax = 0,
    //     .ebx = 1,
    //     .ecx = 2,
    //     .edx = 3,
    //     .esi = 4,
    //     .edi = 5,
    //     .ds = GDT_SEGMENT_UDATA | 0x3,
    //     .eip = (uint32_t)userland_main,
    //     .cs = GDT_SEGMENT_UCODE | 0x3,
    //     .eflags = 0,
    //     .esp = (uint32_t)&kernel_stack_top,
    //     .ss = GDT_SEGMENT_UDATA | 0x3,
    // };

    // pit_setup_channel(PIT_CHANNEL_0, PIT_MODE_1, 0);

    // proc_cb_t *pcb = proc_cur();
    // memset(pcb, 0, sizeof(proc_cb_t));
    // pcb->cpu_ctx.ds = GDT_SEGMENT_UDATA | 0x3;
    // pcb->cpu_ctx.eip = (uint32_t)userland_main;
    // pcb->cpu_ctx.cs = GDT_SEGMENT_UCODE | 0x3;
    // pcb->cpu_ctx.eflags = EFLAGS | EFLAGS_IF;
    // // pcb->cpu_ctx.eflags = EFLAGS;
    // pcb->cpu_ctx.esp = (uint32_t)&userland_stack + sizeof(userland_stack);
    // pcb->cpu_ctx.ebp = pcb->cpu_ctx.esp;
    // pcb->cpu_ctx.ss = GDT_SEGMENT_UDATA | 0x3;

    // go_userspace(&pcb->cpu_ctx);

    // kalloc_test();

    while (true)
        ;
}
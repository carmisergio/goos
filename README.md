# GOOS (Grossly Oversimplified Operating System)
[![Hits-of-Code](https://hitsofcode.com/github/carmisergio/goos)](https://hitsofcode.com/github/carmisergio/goos)

GOOS is an extremely simple single-tasking multi-process operating system kernel for X86 machines I'm developing to further my understanding of low level programming and OS architecture and design.


![GOOS running in VirtualBox](https://github.com/user-attachments/assets/38109b9c-2d10-4e74-8de7-f28a26367b82)


#### Main design goals
- Monolithic kernel
- Stack-like process management
- Virtual memory with per-process memory space
- Processes run in user mode
- System calls for kernel access
- Device drivers in kernel space
- Text console
- Program loading from disk

----

## Implemented features

#### Memory management
- [x] Physical memory page allocator
- [x] Virtual memory manager (paging)
- [x] Kernel allocator (malloc-style)

#### I/O and device drivers
- [x] Interrupt and CPU exception handling
  - [x] Programmale Interrupt Controller (PIC)
- [x] Device drivers
  - [x] Programmable Interval Timer (PIT)
  - [x] PS/2 controller driver (8042 KBC)
    - [x] PS/2 keyboard driver
  - [x] Floppy disk controller block device driver (NEC 8272A FDC)
  - [x] ISA DMA handling
  - [x] Serial port driver
  - [x] VGA text-mode display driver
     
#### Console
- [x] Console subsystem
  - [x] ANSI escape code parsing with terminal colors  
- [x] Keyboard subsystem
  - [x] Intermediate keycode representation
  - [x] Localised keymaps
     
#### File system
- [x] Virtual file system
- [x] Block device subsystem
- [x] FAT12 file system driver

#### Process handling
- [x] Process lifecycle management
  - [x] ELF file parsing 
- [x] Per-process Virtual Address Space (VAS)
- [x] System call subsystem

#### Userland
- [x] Custom linker script for userland programs
- [x] Userspace shell
- [x] Example programs
  - [x] Hello world
  - [x] Cowsay clone
  - [x] Terminal graphics Battleships game

----
## Building

The bulid process has been tested only under Linux, but should work fine for macOS and other Unix-like systems without too much hassle. Windows builds are currently unsupported.

##### Requirements
- `i386-elf-gcc`
- `make`

#### Build instructions
The build process automatically compiles the kernel and the example userland programs, and packages them into a bootable floppy image.

Inside the main project directory, invoke `make` to build GOOS.
```bash
make
```
The complete floppy image, including the GRUB bootloader, is now available as `floppy/goos.img`.

## Running
#### In a Virtual Machine
This is by far the easiest option. I've tested GOOS in Oracle VM Virtualbox and QEMU.
To run GOOS in a vm, simply add `goos.img` as an emulated floppy disk in the hypervisor.

A convenience script is provided to make running in QEMU easier.
Execute the following command to compile GOOS and boot the resulting floppy image using `qemu-system-i386`.
```
make run
```

#### On real hardware
One of the main goals I had in mind when developing GOOS was the ability to run it on real hardware. 

The target hardware for the operaing system is a late 90s - early 2000s PC with an i486 or i686 CPU. A floppy disk drive and controller is required.

To boot GOOS on real hardware, simply write the floppy image to a 1.44M flopy disk, and you should be able to boot it.


 

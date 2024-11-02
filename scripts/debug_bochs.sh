#!/bin/bash

# Opens gdb and bochs in parallel, kills bochs when gdb exits

# First argument is the path of the kernel elf
KERNEL_BIN=$1

# Start QEMU
#BXSHARE=/usr/local/share/bochs 
bochs -qf bochsrc & pid_qemu=($!)

# Open serial output file
code bochsserial.txt

# Start GDB
#alacritty --working-directory $PWD -e gdb -ex 'target remote localhost:1234' -q $KERNEL_BIN & pid_gdb=($!)
gdb -ex 'target remote localhost:1235' -ex "set architecture i386:x86-64" -q $KERNEL_BIN

kill -9 $pid_qemu
#!/bin/bash

# Opens gdb and qemu in parallel, kills qemu when gdb exits

# First argument is the path of the kernel elf
KERNEL_BIN=$1

# Start QEMU
qemu-system-i386 -kernel $KERNEL_BIN -m 16M -s -S & pid_qemu=($!)

# Start GDB
#alacritty --working-directory $PWD -e gdb -ex 'target remote localhost:1234' -q $KERNEL_BIN & pid_gdb=($!)
gdb -ex 'target remote localhost:1234' -q $KERNEL_BIN

kill  $pid_qemu
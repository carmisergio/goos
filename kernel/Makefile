KERNEL_BIN:=kernel.elf

CC:=i386-elf-gcc

# Flag defaults
CFLAGS?=-O0 -g
LDFLAGS?=
LIBS?=

# Add required flags
CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra -std=c23
LIBS:=$(LIBS) -nostdlib -lgcc
# There was -lk here but it doesn't compile with it on

LINK_SCRIPT:=linker.ld
LINK_SCRIPT_INT:=linker.ld.i
INCLUDE:=-Iinc -Iklibc/inc
SRC:=src

# Add flags specified by architecture
CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)

KERNEL_OBJS =\
$(SRC)/boot/multiboot.o \
$(SRC)/kernel.o \
$(SRC)/log.o \
$(SRC)/panic.o \
$(SRC)/clock.o \
$(SRC)/sysreq.o \
$(SRC)/mem/mem.o \
$(SRC)/mem/vmem.o \
$(SRC)/mem/physmem.o \
$(SRC)/mem/gdt.o \
$(SRC)/mem/load_gdt.o \
$(SRC)/mem/kalloc.o \
$(SRC)/int/interrupts.o \
$(SRC)/int/idt.o \
$(SRC)/int/vectors.o \
$(SRC)/int/load_idt.o \
$(SRC)/int/exceptions.o \
$(SRC)/drivers/serial.o \
$(SRC)/drivers/vga.o \
$(SRC)/drivers/pic.o \
$(SRC)/drivers/pit.o \
$(SRC)/drivers/kbdctl.o \
$(SRC)/drivers/ps2kbd/ps2kbd.o \
$(SRC)/kbd/kbd.o \
$(SRC)/kbd/keymaps/us_qwerty.o \
$(SRC)/console/console.o \
$(SRC)/console/ascii.o \
$(SRC)/syscall/go_user.o \
$(SRC)/syscall/syscall.o \
$(SRC)/proc/proc.o \

KLIBC_OBJS =\
klibc/string.o \
klibc/mini-printf.o \


OBJS=\
$(SRC)/boot/crti.o \
$(SRC)/boot/crtbegin.o \
$(SRC)/boot/crtend.o \
$(SRC)/boot/crtn.o \
$(SRC)/boot/boot.o \
$(KERNEL_OBJS) \
$(KLIBC_OBJS) \

LINK_LIST=\
$(SRC)/boot/crti.o \
$(SRC)/boot/crtbegin.o \
$(SRC)/boot/boot.o \
$(KERNEL_OBJS) \
$(KLIBC_OBJS) \
$(LIBS) \
$(SRC)/boot/crtend.o \
$(SRC)/boot/crtn.o \

.PHONY: all clean

all: $(KERNEL_BIN)
	
$(KERNEL_BIN): $(OBJS) $(LINK_SCRIPT_INT)
# 	Note: $@ refers to the name of the current target
	$(CC) -T $(LINK_SCRIPT_INT)  -o $@ $(CFLAGS) $(LDFLAGS) $(LINK_LIST) 

# These files are provided by the compiler, copy them to known locations
$(SRC)/boot/crtbegin.o $(SRC)/boot/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@
	
# Preprocess linker script
$(LINK_SCRIPT_INT): $(LINK_SCRIPT)
	$(CC) -E -P -xc -DLINKER_SCRIPT $(LINK_SCRIPT) $(INCLUDE) > $@


%.o: %.c
	$(CC) -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS) $(INCLUDE)

%.o: %.S
	$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS) $(INCLUDE)

clean:
	rm -f $(KERNEL_BIN)
	rm -f $(OBJS) *.o */*.o */*/*.o *.i
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d

# -include $(OBJS:.o=.d)





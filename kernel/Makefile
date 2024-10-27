KERNEL_BIN:=kernel.elf

CC:=i386-elf-gcc

# Flag defaults
CFLAGS?=-O2 -g -Wpedantic -std=c23
LDFLAGS?=
LIBS?=

# Add required flags
CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra
LIBS:=$(LIBS) -nostdlib -lgcc
# There was -lk here but it doesn't compile with it on

LINK_SCRIPT:=linker.ld
LINK_SCRIPT_INT:=linker.ld.i
INCLUDE:=-Iinclude -Iklibc/include

# Add flags specified by architecture
CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)

KERNEL_OBJS =\
boot/multiboot.o \
kernel.o \
log.o \
panic.o \
clock.o \
sysreq.o \
mem/mem.o \
mem/vmem.o \
mem/physmem.o \
mem/gdt.o \
mem/load_gdt.o \
int/interrupts.o \
int/idt.o \
int/vectors.o \
int/load_idt.o \
int/exceptions.o \
drivers/serial.o \
drivers/vga.o \
drivers/pic.o \
drivers/pit.o \
drivers/kbdctl.o \
drivers/ps2kbd/ps2kbd.o \
kbd/kbd.o \
kbd/keymaps/us_qwerty.o \
console/console.o \
console/ascii.o

KLIBC_OBJS =\
klibc/string.o \
klibc/mini-printf.o \


OBJS=\
boot/crti.o \
boot/crtbegin.o \
boot/crtend.o \
boot/crtn.o \
boot/boot.o \
$(KERNEL_OBJS) \
$(KLIBC_OBJS) \

LINK_LIST=\
boot/crti.o \
boot/crtbegin.o \
boot/boot.o \
$(KERNEL_OBJS) \
$(KLIBC_OBJS) \
$(LIBS) \
boot/crtend.o \
boot/crtn.o \

.PHONY: all clean

all: $(KERNEL_BIN)
	
$(KERNEL_BIN): $(OBJS) $(LINK_SCRIPT_INT)
# 	Note: $@ refers to the name of the current target
	$(CC) -T $(LINK_SCRIPT_INT)  -o $@ $(CFLAGS) $(LDFLAGS) $(LINK_LIST) 

# These files are provided by the compiler, copy them to known locations
boot/crtbegin.o boot/crtend.o:
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





KERNEL_BIN:=kernel.elf

CC:=i386-elf-gcc

# Flag defaults
CFLAGS?=-O2 -g
LDFLAGS?=
LIBS?=

# Add required flags
CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra
LIBS:=$(LIBS) -nostdlib -lgcc
# There was -lk here but it doesn't compile with it on

LINK_SCRIPT:=linker.ld
INCLUDE:=-Iinclude -Iklibc/include

# Add flags specified by architecture
CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)

KERNEL_OBJS =\
boot/multiboot.o \
kernel.o \
log.o \
panic.o \
mem/physmem.o \
drivers/serial.o \
drivers/vga.o

KLIBC_OBJS =\
klibc/string.o \


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
	
$(KERNEL_BIN): $(OBJS) $(LINK_SCRIPT)
# 	Note: $@ refers to the name of the current target
	$(CC) -T $(LINK_SCRIPT) -o $@ $(CFLAGS) $(LDFLAGS) $(LINK_LIST) 

# These files are provided by the compiler, copy them to known locations
boot/crtbegin.o boot/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

%.o: %.c
	$(CC) -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS) $(INCLUDE)

%.o: %.s
	$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f $(KERNEL_BIN)
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d

# -include $(OBJS:.o=.d)





# Outputs
# FLOPPY_IMG:= floppy.img

KERNEL_DIR:= kernel
KERNEL_BIN:= $(KERNEL_DIR)/kernel.elf

FLOPPY_DIR:= floppy
FLOPPY_IMG:= $(FLOPPY_DIR)/goos.img
FLPB_IMG:= $(FLOPPY_DIR)/flpb.img
 
PROGRAMS_DIR:=userland/programs
PROGRAMS_BIN:=$(PROGRAMS_DIR)/bin

SCRIPTS_DIR:= scripts

QEMU:=qemu-system-i386
GDB:=gdb

.PHONY: all run debug gw_write bochs clean 

all: $(KERNEL_BIN) $(FLOPPY_IMG)
	
# Build the kernel
$(KERNEL_BIN): FORCE
	$(MAKE) -C $(KERNEL_DIR)
	
# Build the programs
$(PROGRAMS_BIN): FORCE
	$(MAKE) -C $(PROGRAMS_DIR)

# Build the floppy image
$(FLOPPY_IMG): $(KERNEL_BIN) $(PROGRAMS_BIN)
	cp $(FLOPPY_DIR)/grub_base.img $@
	mcopy -s -i $@ $(FLOPPY_DIR)/root/* ::/
	mcopy -i $@ $(KERNEL_BIN) ::/boot/
	mcopy -i $@ $(PROGRAMS_BIN)/* ::/bin/
	
# Dummy floppy image
$(FLPB_IMG): FORCE
	dd if=/dev/zero of=$@ bs=512 count=2880

# Run compiled rernel with QEMU
run: $(KERNEL_BIN) $(FLOPPY_IMG) $(FLPB_IMG)
	$(QEMU) -kernel $(KERNEL_BIN) -fda $(FLOPPY_IMG) -fdb $(FLPB_IMG) -m 16M

# Run compiled rernel with QEMU in debug mode
debug: $(KERNEL_BIN)
	$(SCRIPTS_DIR)/debug.sh $(KERNEL_BIN)
	
# Write generated floppy image with Greaseweazle
gw_write: $(FLOPPY_IMG)
	gw write --format ibm.1440 --drive B --no-verify $(FLOPPY_IMG)
	
# Run floppy image in bochs
bochs: $(FLOPPY_IMG)
	$(SCRIPTS_DIR)/debug_bochs.sh $(KERNEL_BIN)


clean:
	$(MAKE) -C $(KERNEL_DIR) clean
	$(MAKE) -C $(PROGRAMS_DIR) clean
	rm -f $(FLOPPY_IMG) $(FLPB_IMG)

FORCE: ;

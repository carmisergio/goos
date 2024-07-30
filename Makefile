# Outputs
# FLOPPY_IMG:= floppy.img

KERNEL_DIR:= kernel
KERNEL_BIN:= $(KERNEL_DIR)/kernel.elf

FLOPPY_DIR:= floppy
FLOPPY_IMG:= $(FLOPPY_DIR)/goos.img

QEMU:=qemu-system-i386

.PHONY: all run debug gw_write clean

all: $(KERNEL_BIN) $(FLOPPY_IMG)
	
# Build the kernel
$(KERNEL_BIN): FORCE
	$(MAKE) -C $(KERNEL_DIR)

# Build the floppy image
$(FLOPPY_IMG): $(KERNEL_BIN)
	cp $(FLOPPY_DIR)/grub_base.img $@
	mcopy -s -i $@ $(FLOPPY_DIR)/root/* ::/
	mcopy -i $@ $(KERNEL_BIN) ::/boot/
	

# Run compiled rernel with QEMU
run: $(KERNEL_BIN)
	$(QEMU) -kernel $(KERNEL_BIN) -m 16M

# Run compiled rernel with QEMU in debug mode
debug: $(KERNEL_BIN)
	$(QEMU) -kernel $(KERNEL_BIN) -m 16M -s -S 
	
# Write generated floppy image with Greaseweazle
gw_write: $(FLOPPY_IMG)
	gw write --format ibm.1440 --drive B --no-verify $(FLOPPY_IMG)

clean:
	$(MAKE) -C $(KERNEL_DIR) clean
	rm -f $(FLOPPY_IMG)

FORCE: ;

# Flags defaults
CFLAGS?=-O3 -g
LDFLAGS?=

OUT_DIR:=bin

# Programs
PROGS:= \
$(OUT_DIR)/minimal \
$(OUT_DIR)/child \
$(OUT_DIR)/hello \
$(OUT_DIR)/shell \

# Standard library
STDLIB_DIR:=../libc
STDLIB_BIN:=$(STDLIB_DIR)/libc.o
STDLIB_INC:=$(STDLIB_DIR)/inc
LINK_SCRIPT:=../linker.ld

# Programs
CC:=i386-elf-gcc

# Add required flags
CFLAGS:=$(CFLAGS) -Wall -Wextra -std=c23 -ffreestanding
LDFLAGS:=$(LDFLAGS) -T $(LINK_SCRIPT)
LIBS:=-lgcc -nostdlib
INCLUDE:=-I$(STDLIB_INC)

.PHONY: all clean

all: $(PROGS) 
	
$(OUT_DIR)/%: %.c $(STDLIB_BIN)
	mkdir -p $(OUT_DIR)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(INCLUDE) $(LIBS) $< $(STDLIB_BIN)

$(OUT_DIR)/%: %.S $(STDLIB_BIN)
	mkdir -p $(OUT_DIR)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(INCLUDE) $(STDLIB_BIN) $<

$(STDLIB_BIN): FORCE
	$(MAKE) -C $(STDLIB_DIR)
	
clean:
	rm -rf $(OUT_DIR)
	rm -f $(OJBS) *.o */*.o */*/*.o */*/*/*.o
	rm -f $(OJBS:.o=.d) *.d */*.d */*/*.d */*/*/*.d
	$(MAKE) -C $(STDLIB_DIR) clean;

FORCE: ;
# Flags defaults
CFLAGS?=-O3 -g
LDFLAGS?=

# Files
OUT_BIN:=libc.o
SRC:=src
INC:=inc

# Objects
OBJS:= \
$(SRC)/crt0.o \
$(SRC)/syscall.o \
$(SRC)/string.o \
$(SRC)/error.o \
$(SRC)/stdio/console.o \
$(SRC)/stdio/mini-printf.o \
$(SRC)/stdlib/rand.o \
$(SRC)/stdlib/proc.o \
$(SRC)/time.o \
$(SRC)/parse.o \

# Programs
CC:=i386-elf-gcc
AR:=i386-elf-ar

# Add required flags
CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra -std=c23
LIBS:=-lgcc
INCLUDE:=-I$(INC)

.PHONY: all clean

all: $(OUT_BIN)
	
$(OUT_BIN): $(OBJS)
	$(AR) rcs $@ $(OBJS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE)

%.o: %.S
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE)
	
clean:
	rm -f $(OUT_BIN)
	rm -f $(OJBS) *.o */*.o */*/*.o */*/*/*.o
	rm -f $(OJBS:.o=.d) *.d */*.d */*/*.d */*/*/*.d
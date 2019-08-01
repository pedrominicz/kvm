CFLAGS = -pedantic -Wall -Wextra -Werror
# Generate 16-bit code.
CFLAGS += -m16
# No position-independent code.
CFLAGS += -fno-pic -fno-pie
# No extra code to check for buffer overflows, such as stack smashing attacks.
CFLAGS += -fno-stack-protector
# Assert that compilation takes place in a freestanding environment.
CFLAGS += -ffreestanding
# Do not use the standard system startup files or libraries when linking.
CFLAGS += -nostdlib

ASM = $(wildcard *.S)
SRC = $(wildcard *.c)
OBJ = $(ASM:.S=.o) $(SRC:.c=.o)

all: bios.bin

bios.bin: $(OBJ)
	$(LD) -T bios.ld -o bios.bin.elf $(OBJ)
	objcopy -O binary -j .text bios.bin.elf bios.bin

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f bios.bin bios.bin.elf $(OBJ)

.PHONY: all clean

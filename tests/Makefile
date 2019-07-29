SRC = $(wildcard *.asm)
BIN = $(SRC:.asm=.bin)

all: $(BIN)

%.bin: %.asm
	nasm -f bin $< -o $@

clean:
	rm -f $(BIN)

.PHONY: all clean

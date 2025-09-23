# Minimal bare-metal RISC-V kernel Makefile

CC = riscv64-unknown-elf-gcc
CFLAGS = -Wall -O2 -march=rv64gc -mabi=lp64 -ffreestanding -nostdlib -mcmodel=medany

ENTRY   = kernel/entry.S
MAIN    = kernel/main.c
UART    = kernel/uart.c
LD      = kernel/kernel.ld

OBJS = entry.o main.o uart.o

all: kernel.elf

entry.o: $(ENTRY)
	$(CC) $(CFLAGS) -c -o $@ $<

main.o: $(MAIN)
	$(CC) $(CFLAGS) -c -o $@ $<

uart.o: $(UART)
	$(CC) $(CFLAGS) -c -o $@ $<

kernel.elf: $(OBJS)
	$(CC) $(CFLAGS) -T $(LD) -o $@ $(OBJS)

run: kernel.elf
	qemu-system-riscv64 -machine virt -nographic -bios none -kernel kernel.elf

clean:
	rm -f *.o kernel.elf

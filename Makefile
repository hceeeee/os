# Makefile for minimal xv6 kernel

CC=riscv64-unknown-elf-gcc
CFLAGS=-Wall -O2 -march=rv64gc -mabi=lp64 -ffreestanding -nostdlib -mcmodel=medany

ENTRY=kernel/entry.S
MAIN=kernel/main.c
LD=kernel/kernel.ld

all: kernel.elf

entry.o: $(ENTRY)
	$(CC) $(CFLAGS) -c -o $@ $<

main.o: $(MAIN)
	$(CC) $(CFLAGS) -c -o $@ $<

kernel/uart.o: kernel/uart.c
	$(CC) $(CFLAGS) -c -o $@ $<

kernel/console.o: kernel/console.c
	$(CC) $(CFLAGS) -c -o $@ $<

kernel/printf.o: kernel/printf.c
	$(CC) $(CFLAGS) -c -o $@ $<

kernel.elf: entry.o main.o kernel/uart.o kernel/console.o kernel/printf.o
	$(CC) $(CFLAGS) -T $(LD) -o $@ entry.o main.o kernel/uart.o kernel/console.o kernel/printf.o

run: kernel.elf
	qemu-system-riscv64 -machine virt -nographic -kernel kernel.elf

clean:
	rm -f *.o kernel.elf kernel/*.o

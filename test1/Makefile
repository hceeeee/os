CC = riscv64-unknown-elf-gcc
CFLAGS = -Wall -O2 -march=rv64gc -mabi=lp64 -ffreestanding -nostdlib -mcmodel=medany

all: kernel.elf

entry.o: kernel/entry.S
	$(CC) $(CFLAGS) -c -o $@ $<

main.o: kernel/main.c
	$(CC) $(CFLAGS) -c -o $@ $<

uart.o: kernel/uart.c
	$(CC) $(CFLAGS) -c -o $@ $<

kernel.elf: entry.o main.o uart.o
	$(CC) $(CFLAGS) -T kernel/kernel.ld -o $@ $^

run: kernel.elf
	qemu-system-riscv64 -machine virt -nographic -bios none -kernel kernel.elf

clean:
	rm -f *.o kernel.elf

CC = riscv64-unknown-elf-gcc
CFLAGS = -Wall -O2 -march=rv64gc -mabi=lp64 -ffreestanding -nostdlib -mcmodel=medany

OBJS = entry.o main.o uart.o console.o printf.o pmm.o pagetable.o kvminit.o

all: kernel.elf

entry.o: kernel/entry.S
	$(CC) $(CFLAGS) -c -o $@ $<

main.o: kernel/main.c kernel/kvminit.h
	$(CC) $(CFLAGS) -c -o $@ $<

uart.o: kernel/uart.c
	$(CC) $(CFLAGS) -c -o $@ $<

console.o: kernel/console.c kernel/console.h
	$(CC) $(CFLAGS) -c -o $@ $<

printf.o: kernel/printf.c
	$(CC) $(CFLAGS) -c -o $@ $<

pmm.o: kernel/pmm.c kernel/pmm.h
	$(CC) $(CFLAGS) -c -o $@ $<

kernel.elf: $(OBJS)
	$(CC) $(CFLAGS) -T kernel/kernel.ld -o $@ $^

pagetable.o: kernel/pagetable.c kernel/pagetable.h kernel/pmm.h
	$(CC) $(CFLAGS) -c -o $@ $<

kvminit.o: kernel/kvminit.c kernel/kvminit.h kernel/pagetable.h kernel/pmm.h
	$(CC) $(CFLAGS) -c -o $@ $<

run: kernel.elf
	qemu-system-riscv64 -machine virt -nographic -bios none -kernel kernel.elf

clean:
	rm -f *.o kernel.elf

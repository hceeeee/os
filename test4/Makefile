CC = riscv64-unknown-elf-gcc
CFLAGS = -Wall -Wextra -O2 -march=rv64gc -mabi=lp64 -ffreestanding -nostdlib -mcmodel=medany -fno-common -Ikernel

OBJS = entry.o trapvec.o mtrapvec.o timervec.o start.o main.o uart.o printf.o trap.o sched.o

all: kernel.elf

entry.o: kernel/entry.S
	$(CC) $(CFLAGS) -c -o $@ $<

trapvec.o: kernel/trapvec.S
	$(CC) $(CFLAGS) -c -o $@ $<

mtrapvec.o: kernel/mtrapvec.S
	$(CC) $(CFLAGS) -c -o $@ $<

timervec.o: kernel/timervec.S
	$(CC) $(CFLAGS) -c -o $@ $<

start.o: kernel/start.c kernel/trap.h
	$(CC) $(CFLAGS) -c -o $@ $<

main.o: kernel/main.c kernel/trap.h
	$(CC) $(CFLAGS) -c -o $@ $<

uart.o: kernel/uart.c
	$(CC) $(CFLAGS) -c -o $@ $<

printf.o: kernel/printf.c
	$(CC) $(CFLAGS) -c -o $@ $<

trap.o: kernel/trap.c kernel/trap.h kernel/riscv.h kernel/sbi.h
	$(CC) $(CFLAGS) -c -o $@ $<

sched.o: kernel/sched.c
	$(CC) $(CFLAGS) -c -o $@ $<

kernel.elf: $(OBJS)
	$(CC) $(CFLAGS) -T kernel/kernel.ld -o $@ $^ -lgcc

run: kernel.elf
	qemu-system-riscv64 -machine virt -nographic -bios none -kernel kernel.elf

clean:
	rm -f *.o kernel.elf
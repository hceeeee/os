#include <stdint.h>

#define UART0 0x10000000L

// 16550A register offsets
#define RBR 0x00 // Receiver Buffer Register (read)
#define THR 0x00 // Transmitter Holding Register (write)
#define IER 0x01 // Interrupt Enable Register
#define FCR 0x02 // FIFO Control Register
#define LCR 0x03 // Line Control Register
#define MCR 0x04 // Modem Control Register
#define LSR 0x05 // Line Status Register
#define DLL 0x00 // Divisor Latch Low
#define DLM 0x01 // Divisor Latch High

#define LCR_DLAB 0x80
#define LCR_8N1  0x03
#define LSR_TX_EMPTY (1 << 5)

static inline void mmio_write(uint64_t addr, uint8_t val) {
	volatile uint8_t *p = (volatile uint8_t *)addr;
	*p = val;
}

static inline uint8_t mmio_read(uint64_t addr) {
	volatile uint8_t *p = (volatile uint8_t *)addr;
	return *p;
}

void uart_init(void) {
	// Disable interrupts
	mmio_write(UART0 + IER, 0x00);
	// Enable DLAB to set baud divisor
	mmio_write(UART0 + LCR, LCR_DLAB);
	// For QEMU default clock 1.8432MHz, divisor=1 -> 115200
	mmio_write(UART0 + DLL, 0x01);
	mmio_write(UART0 + DLM, 0x00);
	// 8 bits, no parity, one stop
	mmio_write(UART0 + LCR, LCR_8N1);
	// Enable FIFO, clear RX/TX
	mmio_write(UART0 + FCR, 0x07);
	// Modem control: DTR/RTS set
	mmio_write(UART0 + MCR, 0x03);
}

void uart_putc_sync(int c) {
	// Wait until THR empty
	while ((mmio_read(UART0 + LSR) & LSR_TX_EMPTY) == 0) { }
	mmio_write(UART0 + THR, (uint8_t)c);
}

void uart_puts(const char *s) {
	while (*s) {
		uart_putc_sync((unsigned char)*s++);
	}
}

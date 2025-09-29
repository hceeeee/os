/* Minimal UART driver for QEMU virt (ns16550 compatible) */
#include <stdint.h>

/* uart8250 MMIO base on QEMU virt */
#define UART0_BASE 0x10000000UL
static volatile uint8_t *const uart = (volatile uint8_t *)UART0_BASE;

/* Offsets for 16550-style UART */
#define UART_RBR 0   /* RX buffer (read) / THR (write) */
#define UART_THR 0
#define UART_LSR 5   /* Line Status Register */

void uart_putc(char c) {
    /* Wait for Transmitter Holding Register empty: LSR bit 5 (0x20) */
    while (!(uart[UART_LSR] & 0x20)) {
        /* spin */
    }
    uart[UART_THR] = (uint8_t)c;
}

/* write NUL-terminated string */
void uart_puts(const char *s) {
    if (!s) return;
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r'); /* 可选：确保终端换行 */
        }
        uart_putc(*s++);
    }
}

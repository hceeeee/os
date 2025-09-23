#include <stdint.h>

void uart_init(void);
void uart_putc_sync(int c);

void console_init(void) {
	uart_init();
}

void consputc(int c) {
	if (c == '\n') {
		uart_putc_sync('\r');
	}
	uart_putc_sync(c);
}

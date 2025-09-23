/* Minimal main: call uart_puts and then loop */
extern void uart_puts(const char *s);

void main(void) {
    uart_puts("Hello, OS\n");

    /* 进入死循环，不返回 */
    for (;;) {
        asm volatile ("wfi");
    }
}

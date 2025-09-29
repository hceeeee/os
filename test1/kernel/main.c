/* Minimal main: call uart_puts and then loop */
extern void uart_puts(const char *s);

void main(void) {
    uart_puts("Hello, OS\n");
    for (;;) {//死循环
        asm volatile ("wfi");//进入低能耗模式 wait for interrupt
    }
}
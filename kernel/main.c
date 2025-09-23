// kernel/main.c
#include <stdint.h>

/*void main() {
    // 测试：在 QEMU 控制台输出
    volatile char *uart = (char *)0x10000000; // QEMU virt UART0
    const char *msg = "Hello OS!\n";
    while (*msg) {
        *uart = *msg++;
    }

    while (1) {}  // 死循环
}*/

// 来自 uart.c 的函数声明
void uart_puts(const char *s);

// 来自 console/printf 的声明
void console_init(void);
void printf(const char *fmt, ...);

static void test_printf_basic() {
	printf("Testing integer: %d\n", 42);
	printf("Testing negative: %d\n", -123);
	printf("Testing zero: %d\n", 0);
	printf("Testing hex: 0x%x\n", 0xABC);
	printf("Testing string: %s\n", "Hello");
	printf("Testing char: %c\n", 'X');
	printf("Testing percent: %%\n");
}

static void test_printf_edge_cases() {
	printf("INT_MAX: %d\n", 2147483647);
	printf("INT_MIN: %d\n", -2147483648);
	printf("NULL string: %s\n", (char*)0);
	printf("Empty string: %s\n", "");
}

void main() {
    // 保持原有输出
    uart_puts("Hello,OS");

    // 初始化控制台后再使用 printf
    console_init();
    test_printf_basic();
    test_printf_edge_cases();

    while (1);
}


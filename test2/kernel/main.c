/* Minimal main: call uart_puts and then loop */
/*extern void uart_puts(const char *s);

void main(void) {
    uart_puts("Hello, OS\n");

/*
    for (;;) {
        asm volatile ("wfi");
    }
}*/

/*#include <stddef.h>
#include <stdint.h>

int printf(const char *fmt, ...);

void test_printf_basic() {
    printf("Testing integer: %d\n", 42);
    printf("Testing negative: %d\n", -123);
    printf("Testing zero: %d\n", 0);
    printf("Testing hex: 0x%x\n", 0xABC);
    printf("Testing string: %s\n", "Hello");
    printf("Testing char: %c\n", 'X');
    printf("Testing percent: %%\n");
}

void test_printf_edge_cases() {
    printf("INT_MAX: %d\n", 2147483647);
    printf("INT_MIN: %d\n", -2147483648);
    printf("NULL string: %s\n", (char*)0);
    printf("Empty string: %s\n", "");
}

void main() {
    test_printf_basic();
    test_printf_edge_cases();

    while (1); 
}
*/

//清屏幕
#include "console.h"

void main() {
    //console_puts("Hello, OS!\n");
    clear_screen();               // 清屏
    goto_xy(5, 10);               // 光标跳到第5行第10列
    console_puts("Hello, OS!\n");

    printf_color(RED, "Red text\n");
    printf_color(GREEN, "Green text\n");

    goto_xy(8, 5);
    clear_line();                  // 清空当前行
    console_puts("Line cleared and overwritten\n");

    while(1); // 防止程序退出
}
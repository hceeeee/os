#include <stddef.h>
#include <stdint.h>
#include "console.h"

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

void test_clear_screen()
{
   //测试清除行以及清屏的效果
    clear_screen();
    goto_xy(5, 1);
    console_puts("AAAAA AAAAA AAAAA AAAAA AAAAA");
    goto_xy(5, 1);
    clear_line();
    console_puts("<CLEARED at line start>");

    goto_xy(7, 1);
    console_puts("LEFT--RIGHT");
    goto_xy(7, 7); // 光标放在两个连字符后面（R 前）
    clear_line();  // 从光标到行尾清掉 "RIGHT"
    console_puts("<TAIL>");
    

}
void test_print_color()
{
    clear_screen();
    // 使用 goto_xy 与 printf_color 验证负数 %d 的着色输出与定位
    goto_xy(5, 7);
    printf_color(RED,   "printf_color %%-d (neg): %d\n", -123);
    goto_xy(7, 7);
    printf("plain printf %%-d (neg): %d\n", -123);
}
void main() {
    test_printf_basic();
    test_printf_edge_cases();
    //test_clear_screen();
    test_print_color();
    while (1); 
}
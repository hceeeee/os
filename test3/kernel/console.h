#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdarg.h>

enum COLOR { RED = 31, GREEN = 32, BLUE = 34, YELLOW = 33, WHITE = 37 };

void console_putc(char c);
void console_puts(const char *s);
void clear_screen(void);
void goto_xy(int row, int col);
void clear_line(void);
int printf_color(enum COLOR color, const char *fmt, ...);

// 声明裸机实现的 vsprintf
int vsprintf(char *buf, const char *fmt, va_list ap);
int sprintf(char *buf, const char *fmt, ...);

#endif

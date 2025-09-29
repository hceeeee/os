#include <stdarg.h>
#include "console.h"

extern void uart_putc(char c);
extern int vsprintf(char *buf, const char *fmt, va_list ap);
extern int sprintf(char *buf, const char *fmt, ...);

void console_putc(char c) { uart_putc(c); }
void console_puts(const char *s) { if(!s)s="(null)"; while(*s)console_putc(*s++); }

void clear_screen(void) { console_puts("\033[2J\033[H"); }

void goto_xy(int row, int col) {
    char buf[16];
    sprintf(buf,"\033[%d;%dH",row,col);
    console_puts(buf);
}

void clear_line(void) { console_puts("\033[K"); }

int printf_color(enum COLOR color,const char *fmt,...) {
    char buf[256]; va_list ap; va_start(ap,fmt);
    int len = vsprintf(buf,fmt,ap); va_end(ap);
    char color_buf[8]; sprintf(color_buf,"\033[%dm",color);
    console_puts(color_buf); console_puts(buf); console_puts("\033[0m");
    return len;
}

#include <stdarg.h>
#include <stdint.h>

// Forward declaration from console.c
void consputc(int c);

static char digits[] = "0123456789abcdef";

static void printint(long long xx, int base, int sign) {
    char buf[32];
    int i = 0;
    unsigned long long x;

    if (sign && (xx < 0)) {
        x = (unsigned long long)(-xx);
        sign = 1;
    } else {
        x = (unsigned long long)xx;
        sign = 0;
    }

    do {
        buf[i++] = digits[x % (unsigned)base];
    } while ((x /= (unsigned)base) != 0);

    if (sign) buf[i++] = '-';

    while (--i >= 0) consputc(buf[i]);
}

static void printptr(uint64_t x) {
    int i;
    consputc('0');
    consputc('x');
    for (i = 0; i < (int)(sizeof(uint64_t) * 2); i++, x <<= 4) {
        consputc(digits[x >> (sizeof(uint64_t) * 8 - 4)]);
    }
}

void vprintf(const char *fmt, va_list ap) {
    int i;
    for (i = 0; fmt && fmt[i]; i++) {
        int cx = fmt[i] & 0xff;
        if (cx != '%') { consputc(cx); continue; }
        i++;
        int c0 = fmt[i] & 0xff;
        int c1 = 0, c2 = 0;
        if (fmt[i]) c1 = fmt[i+1] & 0xff;
        if (fmt[i] && fmt[i+1]) c2 = fmt[i+2] & 0xff;

        if (c0 == 'd') {
            printint((long long)va_arg(ap, int), 10, 1);
        } else if (c0 == 'l' && c1 == 'd') { // %ld
            printint((long long)va_arg(ap, long), 10, 1);
            i += 1;
        } else if (c0 == 'l' && c1 == 'l' && c2 == 'd') { // %lld
            printint((long long)va_arg(ap, long long), 10, 1);
            i += 2;
        } else if (c0 == 'u') {
            printint((long long)(unsigned long long)va_arg(ap, unsigned int), 10, 0);
        } else if (c0 == 'l' && c1 == 'u') { // %lu
            printint((long long)(unsigned long)va_arg(ap, unsigned long), 10, 0);
            i += 1;
        } else if (c0 == 'l' && c1 == 'l' && c2 == 'u') { // %llu
            printint((long long)(unsigned long long)va_arg(ap, unsigned long long), 10, 0);
            i += 2;
        } else if (c0 == 'x') {
            printint((long long)(unsigned long long)va_arg(ap, unsigned int), 16, 0);
        } else if (c0 == 'l' && c1 == 'x') { // %lx
            printint((long long)(unsigned long)va_arg(ap, unsigned long), 16, 0);
            i += 1;
        } else if (c0 == 'l' && c1 == 'l' && c2 == 'x') { // %llx
            printint((long long)(unsigned long long)va_arg(ap, unsigned long long), 16, 0);
            i += 2;
        } else if (c0 == 'p') {
            printptr((uint64_t)va_arg(ap, uint64_t));
        } else if (c0 == 'c') {
            consputc(va_arg(ap, unsigned int) & 0xFF);
        } else if (c0 == 's') {
            const char *s = va_arg(ap, const char*);
            if (s == 0) s = "(null)";
            for (; *s; s++) consputc((unsigned char)*s);
        } else if (c0 == '%') {
            consputc('%');
        } else if (c0 == 0) {
            break;
        } else {
            // Unknown sequence
            consputc('%');
            consputc(c0);
        }
    }
}

void printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
} 
#include <stdarg.h>
#include <stddef.h>

extern void console_putc(char c);
extern void console_puts(const char *s);

typedef void (*putc_fn)(char c, void *ctx);

static void print_unsigned(unsigned long n, int base, putc_fn out, void *ctx) {
    char buf[32];
    int i = 0;

    if (n == 0) {
        out('0', ctx);
        return;
    }

    while (n > 0) {
        int digit = n % base;
        buf[i++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        n /= base;
    }

    while (i-- > 0) {
        out(buf[i], ctx);
    }
}

static void print_signed(long num, int base, putc_fn out, void *ctx) {
    if (num < 0) {
        out('-', ctx);
        print_unsigned(-(unsigned long)num, base, out, ctx);
    } else {
        print_unsigned((unsigned long)num, base, out, ctx);
    }
}

// ???
static void vformat(const char *fmt, va_list ap, putc_fn out, void *ctx) {
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') { out(*p, ctx); continue; }
        p++;
        int alt_form = 0;   // '#'
        int long_flag = 0;  // 'l'
        // parse flags
        while (*p == '#') { alt_form = 1; p++; }
        // parse length
        while (*p == 'l') {
            long_flag = 1;
            p++;
        }
        switch (*p) {
            case 'd':
                if (long_flag) {
                    print_signed(va_arg(ap,long), 10, out, ctx);
                } else {
                    print_signed(va_arg(ap,int), 10, out, ctx);
                }
                break;
            case 'u':
                if (long_flag) {
                    print_unsigned(va_arg(ap,unsigned long), 10, out, ctx);
                } else {
                    print_unsigned(va_arg(ap,unsigned int), 10, out, ctx);
                }
                break;
            case 'x':
                if (alt_form) { out('0', ctx); out('x', ctx); }
                if (long_flag) {
                    print_unsigned(va_arg(ap,unsigned long), 16, out, ctx);
                } else {
                    print_unsigned(va_arg(ap,unsigned int), 16, out, ctx);
                }
                break;
            case 'p': {
                unsigned long v = (unsigned long)va_arg(ap, void*);
                out('0', ctx); out('x', ctx);
                print_unsigned(v, 16, out, ctx);
                break;
            }
            case 's': {
                char *s = va_arg(ap,char*); if(!s) s="(null)";
                while(*s) out(*s++, ctx);
                break;
            }
            case 'c': out((char)va_arg(ap,int), ctx); break;
            case '%': out('%', ctx); break;
            default: out('%', ctx); out(*p, ctx); break;
        }
    }
}

// printf: ? console_putc
static void console_out(char c, void *ctx) {
    (void)ctx;
    console_putc(c);
}
int printf(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vformat(fmt, ap, console_out, NULL);
    va_end(ap);
    return 0;
}

// vsprintf: 
struct buf_ctx { char *p; };
static void buf_out(char c, void *ctx) { *(((struct buf_ctx*)ctx)->p)++ = c; }
int vsprintf(char *buf, const char *fmt, va_list ap) {
    struct buf_ctx ctx = { buf };
    vformat(fmt, ap, buf_out, &ctx);
    *ctx.p = '\0';
    return (int)(ctx.p - buf);
}

int sprintf(char *buf, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int len = vsprintf(buf, fmt, ap);
    va_end(ap);
    return len;
}
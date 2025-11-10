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

// 通用格式化
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

// printf: 使用 console_putc
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

// vsprintf: 输出到缓冲区
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


/*
// 数字输出函数
static void print_number(long num, int base, int sign) {
    char buf[32];
    int i = 0;
    unsigned long n;

    if (sign && num < 0) {
        console_putc('-');
        n = -(unsigned long)num;   // 注意 INT_MIN
    } else {
        n = (unsigned long)num;
    }

    if (n == 0) {
        console_putc('0');
        return;
    }

    while (n > 0) {
        int digit = n % base;
        buf[i++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        n /= base;
    }

    while (i-- > 0) console_putc(buf[i]);
}

// printf 输出到控制台
int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') { console_putc(*p); continue; }

        p++; // skip %
        switch (*p) {
            case 'd': print_number(va_arg(ap,int),10,1); break;
            case 'x': print_number(va_arg(ap,int),16,0); break;
            case 's': {
                char *s = va_arg(ap,char*);
                if (!s) s="(null)";
                while (*s) console_putc(*s++);
                break;
            }
            case 'c': console_putc((char)va_arg(ap,int)); break;
            case '%': console_putc('%'); break;
            default: console_putc('%'); console_putc(*p); break;
        }
    }

    va_end(ap);
    return 0;
}

// vsprintf：输出到 buf
int vsprintf(char *buf, const char *fmt, va_list ap) {
    char *b = buf;
    for (const char *p=fmt; *p; p++) {
        if (*p != '%') { *b++ = *p; continue; }
        p++;
        switch (*p) {
            case 'd': {
                int val = va_arg(ap,int);
                char numbuf[32]; int i = 0;
                long tmp = (long)val;
                unsigned long n = (tmp < 0) ? (unsigned long)(-tmp) : (unsigned long)tmp;
                int is_negative = (tmp < 0);

                if (n == 0) numbuf[i++] = '0';
                while (n) { int d = n % 10; n /= 10; numbuf[i++] = (char)('0' + d); }

                if (is_negative) *b++ = '-';
                for (int j = i - 1; j >= 0; j--) *b++ = numbuf[j];
                break;
            }
            case 'x': {
                int val=va_arg(ap,int); char numbuf[32]; int i=0; unsigned int n=val;
                if(n==0) numbuf[i++]='0';
                while(n){ int d=n%16;n/=16;numbuf[i++]=(d<10?'0':'a'+d-10); }
                for(int j=i-1;j>=0;j--) *b++=numbuf[j];
                break;
            }
            case 's': {
                char *s=va_arg(ap,char*); if(!s)s="(null)";
                while(*s)*b++=*s++;
                break;
            }
            case 'c': *b++=(char)va_arg(ap,int); break;
            case '%': *b++='%'; break;
        }
    }
    *b='\0';
    return (int)(b-buf);
}

// sprintf：调用 vsprintf
int sprintf(char *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int len = vsprintf(buf, fmt, ap);
    va_end(ap);
    return len;
}
*/
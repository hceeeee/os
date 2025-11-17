// kernel/string.c
#include <stdarg.h>  // 用于可变参数
#include <stddef.h>  // 用于 size_t

// 辅助函数：将整数转换为字符串（十进制）
static int int_to_str(int num, char *buf, size_t buf_size) {
    if (buf_size == 0) return 0;
    int is_negative = 0;
    size_t len = 0;
    char temp[20];  // 临时存储反转的数字字符串

    // 处理负数
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    // 处理 0 的特殊情况
    if (num == 0) {
        temp[len++] = '0';
    } else {
        // 提取数字的每一位（反转存储）
        while (num > 0 && len < sizeof(temp) - 1) {
            temp[len++] = '0' + (num % 10);
            num /= 10;
        }
    }

    // 检查缓冲区是否足够
    size_t total_len = len + is_negative;
    if (total_len >= buf_size) {
        return 0;  // 缓冲区不足，返回 0（简化处理）
    }

    // 写入负号（如果需要）
    int buf_idx = 0;
    if (is_negative) {
        buf[buf_idx++] = '-';
    }

    // 反转写入数字（恢复正确顺序）
    for (int i = len - 1; i >= 0; i--) {
        buf[buf_idx++] = temp[i];
    }

    return buf_idx;  // 返回写入的字符数
}

// 格式化字符串到缓冲区（简化版，支持 %d、%s、%c、%%）
int snprintf(char *buf, size_t size, const char *fmt, ...) {
    if (buf == NULL || size == 0) return 0;  // 无效缓冲区
    va_list args;
    va_start(args, fmt);

    size_t buf_idx = 0;  // 当前缓冲区写入位置
    const char *f = fmt;

    while (*f != '\0' && buf_idx < size - 1) {  // 留一个位置给 '\0'
        if (*f == '%') {
            f++;  // 跳过 '%'
            switch (*f) {
                case 'd': {  // 整数
                    int num = va_arg(args, int);
                    char int_buf[20];
                    int int_len = int_to_str(num, int_buf, sizeof(int_buf));
                    // 复制到输出缓冲区
                    for (int i = 0; i < int_len && buf_idx < size - 1; i++) {
                        buf[buf_idx++] = int_buf[i];
                    }
                    break;
                }
                case 's': {  // 字符串
                    char *str = va_arg(args, char*);
                    if (str == NULL) str = "(null)";  // 处理空指针
                    while (*str != '\0' && buf_idx < size - 1) {
                        buf[buf_idx++] = *str++;
                    }
                    break;
                }
                case 'c': {  // 字符
                    char c = (char)va_arg(args, int);  // char 提升为 int
                    buf[buf_idx++] = c;
                    break;
                }
                case '%': {  // 输出 '%'
                    buf[buf_idx++] = '%';
                    break;
                }
                default: {  // 未知格式符，直接输出
                    buf[buf_idx++] = '%';
                    buf[buf_idx++] = *f;
                    break;
                }
            }
            f++;  // 移动到下一个字符
        } else {
            // 普通字符，直接写入
            buf[buf_idx++] = *f++;
        }
    }

    buf[buf_idx] = '\0';  // 确保字符串结束
    va_end(args);
    return buf_idx;  // 返回写入的字符数（不含 '\0'）
}
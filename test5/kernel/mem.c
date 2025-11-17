// kernel/mem.c
#include <stddef.h>  // 用于 size_t 类型

// 将 s 指向的内存块的前 n 个字节设置为值 c（无符号字符）
void *memset(void *s, int c, size_t n) {
    unsigned char *ptr = (unsigned char *)s;  // 转换为字节指针
    for (size_t i = 0; i < n; i++) {
        ptr[i] = (unsigned char)c;  // 填充每个字节
    }
    return s;  // 返回原始指针（符合标准库行为）
}
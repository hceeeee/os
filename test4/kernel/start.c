// kernel/start.c
#include "riscv.h"
#include "trap.h"
#include <stdio.h>

// 如使用你现有的 uart.c 提供的 putc/printf，请包含相应头
extern void uart_init(void);

void kmain(void); // 由 main.c 提供

void _start(void) {
  // 早期初始化：串口、陷阱、时钟
  uart_init();

  // 开启 S 态时钟中断
  trap_init();

  // 跳到 C 的主入口
  kmain();

  // 不返回
  while (1) { asm volatile("wfi"); }
}

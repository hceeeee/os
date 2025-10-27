// kernel/main.c
#include <stdio.h>
#include <stdint.h>
#include "trap.h"

// 你给的测试函数（略微改为可编译）
void test_timer_interrupt(void) {
  printf("Testing timer interrupt...\n");

  uint64_t start_time = get_time();
  extern volatile int interrupt_count;
  int last = -1;

  while (interrupt_count < 5) {
    if (interrupt_count != last) {
      printf("Waiting for interrupt %d...\n", interrupt_count + 1);
      last = interrupt_count;
    }
    // 简单忙等
    for (volatile int i = 0; i < 100000; i++);
  }

  uint64_t end_time = get_time();
  printf("Timer test completed: %d interrupts in %lu cycles\n",
         interrupt_count, (unsigned long)(end_time - start_time));
}

void kmain(void){
  printf("Kernel start.\n");
  test_timer_interrupt();
  printf("Done. Entering WFI loop.\n");
  for(;;) asm volatile("wfi");
}

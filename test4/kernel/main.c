// kernel/main.c
#include "trap.h"
#include "riscv.h"
#include <stdint.h>
#include <stdio.h>

// 软件中断性能测试所需的计数器与处理器
static volatile int sw_counter = 0;
static void sw_handler(void) {
  w_sip(r_sip() & ~SIP_SSIP);
  sw_counter++;
}

void test_timer_interrupt(void) {
  printf("Testing timer interrupt...\n");

  uint64_t start_time = get_time();
  volatile int interrupt_count = 0;
  timer_set_counter(&interrupt_count);
  int last = -1;

  while (interrupt_count < 5) {
    if (interrupt_count != last) {
      printf("Waiting for interrupt %d...\n", interrupt_count + 1);
      last = interrupt_count;
    }
    for (volatile int i = 0; i < 100000; i++) {
      __asm__ volatile("");
    }
  }

  uint64_t end_time = get_time();
  printf("Timer test completed: %d interrupts in %lu cycles\n",
         interrupt_count, (unsigned long)(end_time - start_time));

  timer_set_counter(NULL);
}

static void test_exception_handling(void) {
  printf("Testing exception handling...\n");
  // 非法指令：注入一个无效指令编码
  asm volatile(".word 0x00000000");
  // 访问异常：访问明显无效的物理地址以触发访问故障
  volatile uint64_t *bad = (volatile uint64_t *)0xFFFFFFFFFFFFFFFFULL;
  (void)*bad;
  printf("Exception tests completed\n");
}

static void test_interrupt_overhead(void) {
  printf("Testing interrupt overhead...\n");
  extern void register_interrupt(int irq, void (*h)(void));
  extern void enable_interrupt(int irq);

  register_interrupt(SCAUSE_SUPERVISOR_SOFTWARE, sw_handler);
  enable_interrupt(SCAUSE_SUPERVISOR_SOFTWARE);

  uint64_t total = 0;
  const int rounds = 200;
  sw_counter = 0;
  for (int i = 0; i < rounds; i++) {
    uint64_t t0 = get_time();
    // 置位 SSIP 触发软件中断
    w_sip(r_sip() | SIP_SSIP);
    // 增加超时保护，避免偶发死等
    while (sw_counter <= i) {
      if ((get_time() - t0) > (10000000ULL / 10)) { // >100ms
        printf("WARN: interrupt wait timeout (basic) i=%d\n", i);
        break;
      }
      asm volatile("");
    }
    uint64_t t1 = get_time();
    total += (t1 - t0);
  }
  printf("Interrupt overhead avg: %lu cycles\n", (unsigned long)(total / rounds));

  // 2) 带调度的中断开销（调用 yield），测“上下文切换成本”
  extern void yield(void);
  void sw_handler_yield(void) {
    w_sip(r_sip() & ~SIP_SSIP);
    yield();
    sw_counter++;
  }
  register_interrupt(SCAUSE_SUPERVISOR_SOFTWARE, sw_handler_yield);
  total = 0;
  sw_counter = 0;
  for (int i = 0; i < rounds; i++) {
    uint64_t t0 = get_time();
    w_sip(r_sip() | SIP_SSIP);
    while (sw_counter <= i) {
      if ((get_time() - t0) > (10000000ULL / 10)) { // >100ms
        printf("WARN: interrupt wait timeout (yield) i=%d\n", i);
        break;
      }
      asm volatile("");
    }
    uint64_t t1 = get_time();
    total += (t1 - t0);
  }
  printf("Interrupt+yield overhead avg: %lu cycles\n", (unsigned long)(total / rounds));

  // 3) 频率对性能的影响：固定时间窗口内的中断次数与主循环迭代次数
  const uint64_t TIMEBASE_HZ = 10000000ULL;
  const uint32_t freqs[] = {50, 100, 200, 500, 1000};
  // 使用轻量 handler，避免引入调度扰动
  register_interrupt(SCAUSE_SUPERVISOR_SOFTWARE, sw_handler);
  for (unsigned fi = 0; fi < sizeof(freqs)/sizeof(freqs[0]); ++fi) {
    uint32_t hz = freqs[fi];
    uint64_t period = TIMEBASE_HZ / (uint64_t)hz;
    uint64_t window = TIMEBASE_HZ / 20; // 50ms 窗口
    uint64_t start = get_time();
    uint64_t end = start + window;
    uint64_t next_fire = start;
    volatile uint64_t iterations = 0;
    sw_counter = 0;
    while (1) {
      uint64_t now = get_time();
      if (now >= end) break;
      if (now >= next_fire) {
        w_sip(r_sip() | SIP_SSIP);
        // 防止抖动累积：用 while 保证跟上当前时间
        do { next_fire += period; } while (now >= next_fire);
      }
      iterations++; // 模拟主循环工作
    }
    printf("Freq %u Hz: interrupts=%d iterations=%lu\n",
           hz, sw_counter, (unsigned long)iterations);
  }
}

void kmain(void) {
  printf("Kernel start.\n");
  test_timer_interrupt();
  test_exception_handling();
  test_interrupt_overhead();
  printf("Done. Entering WFI loop.\n");
  for (;;) {
    asm volatile("wfi");
  }
}
// kernel/trap.c
#include "riscv.h"
#include "sbi.h"
#include "trap.h"
#include <stddef.h>

// ---------------- 中断向量表（简化：按 scause code 编号） ----------------
#define MAX_IRQ 64
static interrupt_handler_t ivt[MAX_IRQ];

static volatile int ticks = 0;           // 全局时钟节拍计数（可用于调度片）
volatile int interrupt_count = 0;        // 供测试打印使用

// ---------------- 对外 API ----------------
void register_interrupt(int irq, interrupt_handler_t h){
  if(irq >=0 && irq < MAX_IRQ) ivt[irq] = h;
}

void enable_interrupt(int irq){
  // 我们只按位打开 sie 的三类：软件/时钟/外部。细粒度来源的 enable 放到设备驱动各自做。
  uint64_t sie = r_sie();
  if(irq == SCAUSE_SUPERVISOR_TIMER) sie |= SIE_STIE;
  // 你可扩展：外部中断 SIE_SEIE、软件中断 SIE_SSIE
  w_sie(sie);
}

void disable_interrupt(int irq){
  uint64_t sie = r_sie();
  if(irq == SCAUSE_SUPERVISOR_TIMER) sie &= ~SIE_STIE;
  w_sie(sie);
}

uint64_t get_time(void){
  return r_time(); // 直接读 time CSR
}

// ---------------- 时钟处理与 tick 设置 ----------------
#define TIMEBASE_HZ 10000000ULL   // QEMU virt 常见 10 MHz（如有不同请按平台改）
#define HZ          100           // 100Hz 调度时钟
#define TICK_CYCLES (TIMEBASE_HZ / HZ)

static void set_next_timer(void){
  uint64_t now = get_time();
  sbi_set_timer(now + TICK_CYCLES);
}

void timer_interrupt(void){
  // 1. 更新内核时间
  ticks++;
  interrupt_count++;   // 供测试

  // 2. TODO: 定时器事件处理（超时队列等）

  // 3. 触发调度（占位：可按片轮转、优先级等）
  // if (should_yield()) yield();

  // 4. 预约下次中断
  set_next_timer();
}

// ---------------- 陷入初始化 ----------------
extern void kernelvec(void);

void trap_init(void){
  // 设置 S 态陷阱入口为 kernelvec
  w_stvec((uint64_t)kernelvec);

  // 启用定时中断（外加全局中断位）
  enable_interrupt(SCAUSE_SUPERVISOR_TIMER);
  intr_on();

  // 设置第一次时钟
  set_next_timer();

  // 注册时钟中断 handler（可选：也可在 devintr 里直接调用）
  register_interrupt(SCAUSE_SUPERVISOR_TIMER, timer_interrupt);
}

// ---------------- 内核陷入/分发逻辑 ----------------
int devintr(void){
  uint64_t sc = r_scause();
  if ((sc & SCAUSE_INTR_MASK) && SCAUSE_CODE(sc) == SCAUSE_SUPERVISOR_TIMER) {
    // 调用 IVT（若已注册），否则默认处理
    if (ivt[SCAUSE_SUPERVISOR_TIMER])
      ivt[SCAUSE_SUPERVISOR_TIMER]();
    else
      timer_interrupt();
    return 1;
  }
  // TODO: 外部中断、软件中断、串口……
  return 0;
}

void kerneltrap(void){
  // 这里处于 S 态，来自内核态或用户态的陷入；我们仅处理中断，异常简单打印
  uint64_t sc = r_scause();
  if (sc & SCAUSE_INTR_MASK) {
    if(!devintr()){
      // 未识别的中断
      // 可以加调试输出
    }
  } else {
    // 异常：比如非法访问/页错误等
    // 先简单处理：跳过或打印
    // 在教学阶段你可以 uart_puts/printf 定位问题
  }
}

void usertrap(void){
  // 需要用户态支持/切换，这里留空或重用 kerneltrap
  kerneltrap();
}

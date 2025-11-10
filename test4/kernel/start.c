// kernel/start.c
#include "riscv.h"
#include "trap.h"
#include <stdint.h>

extern void machinevec(void);
extern void timervec(void);
extern void kernelvec(void);

extern void uart_init(void);
extern void uart_puts(const char *s);
extern void uart_putc(char c);
void kmain(void);

static void sstart(void) __attribute__((noreturn));

static void setup_pmp(void) {
  /* 允许 S/U 模式访问整个物理地址空间：使用 NAPOT 全开 */
  w_pmpaddr0(~0ULL >> 2);
  /* pmpcfg0: A=NAPOT(0b11), R/W/X 全开 -> 0x0f */
  uint64_t cfg = 0x0f;
  w_pmpcfg0(cfg);
}
static void uart_put_hex(uint64_t value) {
  static const char digits[] = "0123456789abcdef";
  uart_puts("0x");
  for (int shift = 60; shift >= 0; shift -= 4) {
    uart_putc(digits[(value >> shift) & 0xf]);
  }
  uart_puts("\n");
}

void machine_trap(uint64_t cause, uint64_t epc) {
  uart_init();
  uart_puts("Machine trap!\n");
  uart_puts(" cause=");
  uart_put_hex(cause);
  uart_puts(" mepc=");
  uart_put_hex(epc);
  uart_puts("Halting.\n");
  while (1) {
    asm volatile("wfi");
  }
}

static void delegate_traps(void) {
  uint64_t medeleg = r_medeleg();
  medeleg |= 0xffff; // delegate common synchronous exceptions
  w_medeleg(medeleg);

  uint64_t mideleg = r_mideleg();
  /* 委托 软件/定时器/外部 中断给 S 模式，由 S 模式直接处理并重装时钟 */
  mideleg |= (1UL << SCAUSE_SUPERVISOR_SOFTWARE)
          |  (1UL << SCAUSE_SUPERVISOR_TIMER)
          |  (1UL << SCAUSE_SUPERVISOR_EXTERNAL);
  w_mideleg(mideleg);
}

static void setup_timer_mmio(void) {
  uint64_t hart = r_mhartid();
  volatile uint64_t *mtimecmp = (volatile uint64_t *)CLINT_MTIMECMP(hart);
  volatile uint64_t *mtime = (volatile uint64_t *)CLINT_MTIME;
  const uint64_t now = *mtime;
  const uint64_t interval = (10000000ULL / 100ULL);
  *mtimecmp = now + interval;
}

static void sstart(void) {
  uart_init();
  trap_init();
  kmain();
  while (1) {
    asm volatile("wfi");
  }
}

void start(void) {
  /* Use machine-mode timer vector to forward timer interrupts to S-mode */
  w_mtvec((uint64_t)timervec);
  w_stvec((uint64_t)kernelvec);

  delegate_traps();
  setup_pmp();
  /* 允许 S 模式读取 time 计数器（mcounteren 的 TM 位） */
  w_mcounteren(r_mcounteren() | (1UL << 1));
  w_satp(0);
  w_mie(r_mie() | MIE_MSIE | MIE_MTIE | MIE_MEIE | MIE_SSIE | MIE_STIE | MIE_SEIE);
  w_sie(r_sie() | SIE_STIE | SIE_SSIE | SIE_SEIE);

  setup_timer_mmio();

  uint64_t mstatus = r_mstatus();
  mstatus &= ~MSTATUS_MPP_MASK;
  mstatus |= MSTATUS_MPP_S;
  mstatus |= MSTATUS_MPIE;
  mstatus &= ~MSTATUS_MIE;
  w_mstatus(mstatus);

  uart_init();
  uart_puts("Booting into S-mode...\n");
  uart_puts(" mstatus=");
  uart_put_hex(r_mstatus());

  w_mepc((uint64_t)sstart);
  asm volatile("mret");
}
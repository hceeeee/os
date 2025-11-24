// kernel/trap.c
#include "riscv.h"
#include "trap.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define MAX_IRQ 64

static interrupt_handler_t ivt[MAX_IRQ];
static volatile uint64_t ticks = 0;
extern volatile uint64_t kernel_ticks;
volatile int interrupt_count = 0;
static volatile int *counter_ptr = &interrupt_count;

static const int irq_priority[] = {
    SCAUSE_SUPERVISOR_TIMER,
    SCAUSE_SUPERVISOR_EXTERNAL,
    SCAUSE_SUPERVISOR_SOFTWARE,
};

extern bool should_yield(void);
extern void yield(void);

static inline bool valid_irq(int irq) {
  return irq >= 0 && irq < MAX_IRQ;
}

static inline uint64_t irq_to_sie_bit(int irq) {
  switch (irq) {
    case SCAUSE_SUPERVISOR_SOFTWARE:
      return SIE_SSIE;
    case SCAUSE_SUPERVISOR_TIMER:
      return SIE_STIE;
    case SCAUSE_SUPERVISOR_EXTERNAL:
      return SIE_SEIE;
    default:
      return 0;
  }
}

static inline uint64_t irq_to_sip_bit(int irq) {
  switch (irq) {
    case SCAUSE_SUPERVISOR_SOFTWARE:
      return SIP_SSIP;
    case SCAUSE_SUPERVISOR_TIMER:
      return SIP_STIP;
    case SCAUSE_SUPERVISOR_EXTERNAL:
      return SIP_SEIP;
    default:
      return 0;
  }
}

static bool dispatch_irq(int irq) {
  if (!valid_irq(irq)) {
    return false;
  }
  interrupt_handler_t handler = ivt[irq];
  if (handler) {
    handler();
    return true;
  }
  return false;
}

static int choose_irq(uint64_t scause) {
  if ((scause & SCAUSE_INTR_MASK) == 0) {
    return -1;
  }

  const int cause = (int)SCAUSE_CODE(scause);
  const uint64_t pending = r_sip() & r_sie();
  const uint64_t cause_mask = irq_to_sip_bit(cause);

  if (cause_mask && (pending & cause_mask)) {
    return cause;
  }

  for (size_t i = 0; i < sizeof(irq_priority) / sizeof(irq_priority[0]); ++i) {
    const int candidate = irq_priority[i];
    const uint64_t mask = irq_to_sip_bit(candidate);
    if (mask && (pending & mask)) {
      return candidate;
    }
  }

  return valid_irq(cause) ? cause : -1;
}

void register_interrupt(int irq, interrupt_handler_t handler) {
  if (valid_irq(irq)) {
    ivt[irq] = handler;
  }
}

void unregister_interrupt(int irq) {
  if (valid_irq(irq)) {
    ivt[irq] = NULL;
  }
}

void enable_interrupt(int irq) {
  const uint64_t mask = irq_to_sie_bit(irq);
  if (!mask) {
    return;
  }
  uint64_t sie = r_sie();
  sie |= mask;
  w_sie(sie);
}

void disable_interrupt(int irq) {
  const uint64_t mask = irq_to_sie_bit(irq);
  if (!mask) {
    return;
  }
  uint64_t sie = r_sie();
  sie &= ~mask;
  w_sie(sie);
}

uint64_t get_time(void) { return r_time(); }

#define TIMEBASE_HZ 10000000ULL
#define HZ          100ULL
#define TICK_CYCLES (TIMEBASE_HZ / HZ)

static void set_next_timer(void) {
  const uint64_t now = get_time();
  const uint64_t next = now + TICK_CYCLES;
  /* 单核：使用 hart=0，避免在 S 模式读取 mhartid 触发非法指令 */
  volatile uint64_t *mtimecmp = (volatile uint64_t *)CLINT_MTIMECMP(0);
  *mtimecmp = next;
}

void timer_interrupt(void) {
  ++ticks;
  ++kernel_ticks;
  if (counter_ptr) {
    ++(*counter_ptr);
  }

  if (should_yield()) {
    yield();
  }

  /* 为确保周期性中断，在 S 模式也设置下一次时钟（PMP 已放开） */
  set_next_timer();
}

static void software_interrupt(void) {
  /* 清除 SSIP */
  w_sip(r_sip() & ~SIP_SSIP);
  timer_interrupt();
}

void timer_set_counter(volatile int *counter) {
  if (counter) {
    counter_ptr = counter;
  } else {
    interrupt_count = 0;
    counter_ptr = &interrupt_count;
  }
}

extern void kernelvec(void);

void trap_init(void) {
  intr_off();
  for (int i = 0; i < MAX_IRQ; ++i) {
    ivt[i] = NULL;
  }
  ticks = 0;
  interrupt_count = 0;

  w_sip(r_sip() & ~(SIP_SSIP | SIP_STIP | SIP_SEIP));
  w_stvec((uint64_t)kernelvec);

  register_interrupt(SCAUSE_SUPERVISOR_TIMER, timer_interrupt);
  enable_interrupt(SCAUSE_SUPERVISOR_TIMER);
  /* 同时支持软件中断路径（M 模式 timervec 置位 SSIP 时可用） */
  register_interrupt(SCAUSE_SUPERVISOR_SOFTWARE, software_interrupt);
  enable_interrupt(SCAUSE_SUPERVISOR_SOFTWARE);
  /* 安排第一次时钟中断 */
  set_next_timer();
  intr_on();
}

int devintr(struct trapframe *tf) {
  const int irq = choose_irq(tf->scause);
  if (irq < 0) {
    return 0;
  }
  if (dispatch_irq(irq)) {
    return 1;
  }
  return 0;
}

void kerneltrap(struct trapframe *tf) {
  tf->sepc = r_sepc();
  tf->sstatus = r_sstatus();
  tf->stval = r_stval();
  tf->scause = r_scause();
  tf->reserved = 0;

  if (tf->sstatus & SSTATUS_SIE) {
    printf("kerneltrap: interrupts enabled\n");
  }

  if (tf->scause & SCAUSE_INTR_MASK) {
    if (!devintr(tf)) {
      printf("kerneltrap: unexpected interrupt cause=%lu\n",
             (unsigned long)SCAUSE_CODE(tf->scause));
    }
  } else {
    handle_exception(tf);
  }

  w_sepc(tf->sepc);
}

void usertrap(struct trapframe *tf) {
  kerneltrap(tf);
}

static inline void advance_sepc(struct trapframe *tf, int bytes) {
  tf->sepc += (uint64_t)bytes;
}

void panic(const char *msg) {
  printf("PANIC: %s\n", msg ? msg : "(null)");
  while (1) {
    asm volatile("wfi");
  }
}

static void handle_syscall(struct trapframe *tf) {
  // 简化：仅前移 sepc 跳过 ecall
  advance_sepc(tf, 4);
}

static void handle_illegal_instruction(struct trapframe *tf) {
  printf("Illegal instruction at sepc=%#lx\n", (unsigned long)tf->sepc);
  advance_sepc(tf, 4);
}

static void handle_load_access_fault(struct trapframe *tf) {
  printf("Load access fault at sepc=%#lx addr=%#lx\n",
         (unsigned long)tf->sepc, (unsigned long)tf->stval);
  advance_sepc(tf, 4);
}

static void handle_store_access_fault(struct trapframe *tf) {
  printf("Store access fault at sepc=%#lx addr=%#lx\n",
         (unsigned long)tf->sepc, (unsigned long)tf->stval);
  advance_sepc(tf, 4);
}

void handle_exception(struct trapframe *tf) {
  const uint64_t cause = SCAUSE_CODE(tf->scause);
  switch (cause) {
    case 2:  // illegal instruction
      handle_illegal_instruction(tf);
      break;
    case 5:  // load access fault
      handle_load_access_fault(tf);
      break;
    case 7:  // store/AMO access fault
      handle_store_access_fault(tf);
      break;
    case 8:  // environment call from U-mode (未区分，此处简化)
    case 9:  // environment call from S-mode
      handle_syscall(tf);
      break;
    case 12: // instruction page fault (无页表环境下视作访问故障)
      handle_illegal_instruction(tf);
      break;
    case 13: // load page fault
      handle_load_access_fault(tf);
      break;
    case 15: // store page fault
      handle_store_access_fault(tf);
      break;
    default:
      printf("Unhandled exception: scause=%lu sepc=%#lx stval=%#lx\n",
             (unsigned long)tf->scause,
             (unsigned long)tf->sepc,
             (unsigned long)tf->stval);
      panic("Unknown exception");
      break;
  }
}
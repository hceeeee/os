// kernel/trap.h
#pragma once

#include <stdint.h>

typedef void (*interrupt_handler_t)(void);

#define TRAPFRAME_REGISTER_COUNT 36
#define TRAPFRAME_SIZE           (TRAPFRAME_REGISTER_COUNT * sizeof(uint64_t))

struct trapframe {
  uint64_t ra;
  uint64_t sp;
  uint64_t gp;
  uint64_t tp;
  uint64_t t0;
  uint64_t t1;
  uint64_t t2;
  uint64_t s0;
  uint64_t s1;
  uint64_t a0;
  uint64_t a1;
  uint64_t a2;
  uint64_t a3;
  uint64_t a4;
  uint64_t a5;
  uint64_t a6;
  uint64_t a7;
  uint64_t s2;
  uint64_t s3;
  uint64_t s4;
  uint64_t s5;
  uint64_t s6;
  uint64_t s7;
  uint64_t s8;
  uint64_t s9;
  uint64_t s10;
  uint64_t s11;
  uint64_t t3;
  uint64_t t4;
  uint64_t t5;
  uint64_t t6;
  uint64_t sepc;
  uint64_t sstatus;
  uint64_t stval;
  uint64_t scause;
  uint64_t reserved;  // keeps the structure 16-byte aligned
};

void trap_init(void);
void register_interrupt(int irq, interrupt_handler_t handler);
void unregister_interrupt(int irq);
void enable_interrupt(int irq);
void disable_interrupt(int irq);

uint64_t get_time(void);
void timer_interrupt(void);
void timer_set_counter(volatile int *counter);
extern volatile int interrupt_count;

void kerneltrap(struct trapframe *tf);
void usertrap(struct trapframe *tf);
int  devintr(struct trapframe *tf);

void handle_exception(struct trapframe *tf);
void panic(const char *msg);

_Static_assert(sizeof(struct trapframe) == TRAPFRAME_SIZE,
               "trapframe layout mismatch");
#pragma once

#include <stdint.h>
#include "trap.h"

// Maximum number of processes supported by the tiny kernel.
#define NPROC 16
#define KSTACK_SIZE 4096

// Process states.
enum procstate {
  UNUSED = 0,
  RUNNABLE,
  RUNNING,
  SLEEPING,
  ZOMBIE,
};

struct context {
  uint64_t ra;
  uint64_t sp;
  uint64_t s0;
  uint64_t s1;
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
};

struct spinlock {
  volatile int locked;
};

struct proc {
  struct spinlock lock;
  enum procstate state;
  void *chan;
  int killed;
  int xstate;
  int pid;
  char name[16];
  struct context context;
  void (*entry)(void);
  int parent_pid;
  struct proc *parent;
  uint8_t *kstack;
};

struct cpu {
  struct proc *proc;
  struct context context; // swtch() here to enter scheduler
};

extern struct proc proc_table[NPROC];

void            proc_init(void);
int             create_process(void (*entry)(void));
void            exit_process(int status);
int             wait_process(int *status);
void            scheduler(void) __attribute__((noreturn));
void            yield(void);
struct proc    *myproc(void);
struct cpu     *mycpu(void);
struct proc    *init_bootproc(void);
void            init_lock(struct spinlock *lk);
void            acquire(struct spinlock *lk);
void            release(struct spinlock *lk);
void            sleep_on(void *chan);
void            wakeup(void *chan);
uint64_t        ticks_since_boot(void);
void            scheduler_init(void);
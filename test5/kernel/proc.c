#include "proc.h"
#include "riscv.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

extern void swtch(struct context *old, struct context *new);
// 在 proc.c 顶部添加
extern uint64_t kernel_ticks;
struct proc proc_table[NPROC];
static uint8_t kstacks[NPROC][KSTACK_SIZE];
static struct cpu cpus = {0};
static int nextpid = 1;
static uint8_t scheduler_stack[4096];

void init_lock(struct spinlock *lk) { lk->locked = 0; }
void acquire(struct spinlock *lk) { (void)lk; }
void release(struct spinlock *lk) { (void)lk; }

struct cpu *mycpu(void) { return &cpus; }
struct proc *myproc(void) { return mycpu()->proc; }

static int allocpid(void) { return nextpid++; }

// 关键修改：提前声明 alloc_process 函数
static struct proc *alloc_process(void);

void proc_init(void) {
  for (int i = 0; i < NPROC; ++i) {
    init_lock(&proc_table[i].lock);
    proc_table[i].state = UNUSED;
    proc_table[i].kstack = &kstacks[i][0];
    proc_table[i].parent = NULL;
  }
}

struct proc *init_bootproc(void) {
  struct proc *p = alloc_process();  // 现在此处能正确识别函数声明
  if (!p) {
    return NULL;
  }
  acquire(&p->lock);
  p->state = RUNNING;
  p->parent_pid = 0;
  p->parent = NULL;
  snprintf(p->name, sizeof(p->name), "boot");
  release(&p->lock);
  mycpu()->proc = p;
  return p;
}

// alloc_process 函数定义（保持不变）
static struct proc *alloc_process(void) {
  for (int i = 0; i < NPROC; ++i) {
    struct proc *p = &proc_table[i];
    acquire(&p->lock);
    if (p->state == UNUSED) {
      p->state = RUNNABLE;
      p->pid = allocpid();
      p->killed = 0;
      p->chan = NULL;
      p->entry = NULL;
      p->xstate = 0;
      p->parent_pid = 0;
      p->parent = NULL;
      memset(&p->context, 0, sizeof(p->context));
      release(&p->lock);
      return p;
    }
    release(&p->lock);
  }
  return NULL;
}

// 以下代码保持不变
static void process_trampoline(void) {
  struct proc *p = myproc();
  if (p && p->entry) {
    p->entry();
  }
  exit_process(0);
}

int create_process(void (*entry)(void)) {
  struct proc *parent = myproc();
  struct proc *p = alloc_process();
  if (!p) {
    return -1;
  }
  p->entry = entry;
  p->parent_pid = parent ? parent->pid : 0;
  p->parent = parent;
  snprintf(p->name, sizeof(p->name), "proc%d", p->pid);

  // set up stack/context
  uint64_t sp = (uint64_t)(p->kstack + KSTACK_SIZE);
  p->context.sp = sp;
  p->context.ra = (uint64_t)process_trampoline;
  return p->pid;
}

void scheduler_init(void) {
  struct cpu *c = mycpu();
  memset(&c->context, 0, sizeof(c->context));
  c->context.sp = (uint64_t)(scheduler_stack + sizeof(scheduler_stack));
  c->context.ra = (uint64_t)scheduler;
}

void scheduler(void) {
  struct cpu *c = mycpu();
  for (;;) {
    intr_on();
    for (int i = 0; i < NPROC; ++i) {
      struct proc *p = &proc_table[i];
      acquire(&p->lock);
      if (p->state == RUNNABLE) {
        p->state = RUNNING;
        c->proc = p;
        swtch(&c->context, &p->context);
        c->proc = NULL;
      }
      release(&p->lock);
    }
  }
}

static void sched(void) {
  struct proc *p = myproc();
  struct cpu *c = mycpu();
  if (!p) return;
  swtch(&p->context, &c->context);
}

/*void yield(void) {
  struct proc *p = myproc();
  if (!p) return;
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}*/

void exit_process(int status) {
  struct proc *p = myproc();
  if (!p) {
    return;
  }
  acquire(&p->lock);
  p->xstate = status;
  p->state = ZOMBIE;
  if (p->parent) {
    wakeup(p->parent);
  }
  sched();
  // should not return
  release(&p->lock);
  while (1) {}
}

int wait_process(int *status) {
  struct proc *p = myproc();
  int havekids;
  for (;;) {
    havekids = 0;
    for (int i = 0; i < NPROC; ++i) {
      struct proc *cp = &proc_table[i];
      acquire(&cp->lock);
      if (cp->parent == p) {
        havekids = 1;
        if (cp->state == ZOMBIE) {
          int pid = cp->pid;
          if (status) {
            *status = cp->xstate;
          }
          cp->state = UNUSED;
          cp->parent = NULL;
          release(&cp->lock);
          return pid;
        }
      }
      release(&cp->lock);
    }
    if (!havekids) {
      return -1;
    }
    sleep_on(p);
  }
}

void sleep_on(void *chan) {
  struct proc *p = myproc();
  if (!p) return;
  acquire(&p->lock);
  p->chan = chan;
  p->state = SLEEPING;
  sched();
  p->chan = NULL;
  release(&p->lock);
}

void wakeup(void *chan) {
  for (int i = 0; i < NPROC; ++i) {
    struct proc *p = &proc_table[i];
    acquire(&p->lock);
    if (p->state == SLEEPING && p->chan == chan) {
      p->state = RUNNABLE;
    }
    release(&p->lock);
  }
}

// Expose tick count so that tests can measure scheduler progress.
extern uint64_t kernel_ticks;
uint64_t ticks_since_boot(void) { return kernel_ticks; }
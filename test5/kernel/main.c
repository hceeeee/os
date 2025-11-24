// kernel/main.c for process management and scheduling demo
#include "proc.h"
#include "trap.h"
#include <stdint.h>
#include <stdio.h>
volatile uint64_t kernel_ticks = 0;  // 全局变量，记录时钟 ticks
// Simple delay based on timer ticks.
static void sleep_ticks(uint64_t ticks) {
  const uint64_t target = ticks_since_boot() + ticks;
  while (ticks_since_boot() < target) {
    yield();
  }
}

static void simple_task(void) {
  printf("simple_task running pid=%d\n", myproc()->pid);
  for (int i = 0; i < 3; ++i) {
    printf("simple_task step %d\n", i);
    yield();
  }
  printf("simple_task exiting\n");
  exit_process(0);
}

static void cpu_intensive_task(void) {
  uint64_t start = get_time();
  volatile uint64_t sum = 0;
  for (uint64_t i = 0; i < 100000; ++i) {
    sum += i;
    if ((i % 20000) == 0) {
      yield();
    }
  }
  printf("cpu task pid=%d sum=%lu cycles=%lu\n", myproc()->pid,
         (unsigned long)sum, (unsigned long)(get_time() - start));
  exit_process(0);
}

// ---------- Simple producer/consumer demo ----------
#define BUF_SIZE 8
static int buffer[BUF_SIZE];
static int head = 0, tail = 0, count = 0;
static struct spinlock buf_lock;

static void shared_buffer_init(void) {
  init_lock(&buf_lock);
  head = tail = count = 0;
}

static void buf_put(int v) {
  acquire(&buf_lock);
  while (count == BUF_SIZE) {
    sleep_on(&buffer, &buf_lock);
  }
  buffer[tail] = v;
  tail = (tail + 1) % BUF_SIZE;
  count++;
  release(&buf_lock);
  wakeup(&buffer);
}

static int buf_get(void) {
  acquire(&buf_lock);
  while (count == 0) {
    sleep_on(&buffer, &buf_lock);
  }
  int v = buffer[head];
  head = (head + 1) % BUF_SIZE;
  count--;
  release(&buf_lock);
  wakeup(&buffer);
  return v;
}

static void producer_task(void) {
  for (int i = 0; i < 5; ++i) {
    buf_put(i);
    printf("produced %d\n", i);
  }
  exit_process(0);
}

static void consumer_task(void) {
  for (int i = 0; i < 5; ++i) {
    int v = buf_get();
    printf("consumed %d\n", v);
  }
  exit_process(0);
}

static void test_process_creation(void) {
  printf("Testing process creation...\n");
  int pid = create_process(simple_task);
  printf("created pid %d\n", pid);
  int count_created = 0;
  for (int i = 0; i < NPROC + 5; ++i) {
    int npid = create_process(simple_task);
    if (npid > 0) {
      count_created++;
    } else {
      break;
    }
  }
  printf("Created %d processes in batch\n", count_created);
  for (int i = 0; i < count_created + 1; ++i) {
    wait_process(NULL);
  }
}

static void test_scheduler(void) {
  printf("Testing scheduler...\n");
  for (int i = 0; i < 3; ++i) {
    create_process(cpu_intensive_task);
  }
  uint64_t start = get_time();
  sleep_ticks(100); // about 1s (given HZ=100)
  uint64_t end = get_time();
  for (int i = 0; i < 3; ++i) {
    wait_process(NULL);
  }
  printf("Scheduler test completed in %lu cycles\n", (unsigned long)(end - start));
}

static void test_synchronization(void) {
  printf("Testing synchronization...\n");
  shared_buffer_init();
  create_process(producer_task);
  create_process(consumer_task);
  wait_process(NULL);
  wait_process(NULL);
  printf("Synchronization test completed\n");
}

void kmain(void) {
  printf("Kernel start.\n");
  proc_init();
  scheduler_init();
  init_bootproc();

  test_process_creation();
  test_scheduler();
  test_synchronization();
  debug_proc_table();

  printf("All tests done. Entering scheduler loop.\n");
  // keep running scheduler
  yield();
  scheduler();
}
// kernel/sched.c
#include <stdbool.h>

static int slice = 0;

bool should_yield(void) {
  // Hand back the CPU every 10 timer ticks.
  return (++slice % 10) == 0;
}

void yield(void) {
  // Scheduler placeholder. Hook real context switching logic here.
}
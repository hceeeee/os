// kernel/sched.c
#include <stdbool.h>

static int slice = 0;

bool should_yield(void) {
  // Hand back the CPU every 10 timer ticks.
  return (++slice % 10) == 0;
}
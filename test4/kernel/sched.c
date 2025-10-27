// kernel/sched.c
#include <stdbool.h>

static int slice = 0;
bool should_yield(void){
  // 简易：每 10 个 tick 让出一次 CPU
  return (++slice % 10) == 0;
}

void yield(void){
  // TODO：保存当前进程上下文，选择下一个进程，切换……
}

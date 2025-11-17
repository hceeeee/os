// kernel/sbi.h
#pragma once

#include <stdint.h>

static inline long sbi_call(long extension, long function, long arg0,
                            long arg1, long arg2) {
  register long a0 asm("a0") = arg0;
  register long a1 asm("a1") = arg1;
  register long a2 asm("a2") = arg2;
  register long a6 asm("a6") = function;
  register long a7 asm("a7") = extension;
  asm volatile("ecall"
               : "+r"(a0)
               : "r"(a1), "r"(a2), "r"(a6), "r"(a7)
               : "memory");
  return a0;
}

static inline void sbi_set_timer(uint64_t stime_value) {
  const long SBI_EXT_TIMER = 0x54494D45; // "TIME"
  const long SBI_TIMER_SET_TIMER = 0;
  sbi_call(SBI_EXT_TIMER, SBI_TIMER_SET_TIMER, (long)stime_value, 0, 0);
}
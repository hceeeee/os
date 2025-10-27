// kernel/riscv.h
#pragma once
#include <stdint.h>

// ---------------- CSR helpers ----------------
static inline uint64_t r_sstatus(void){ uint64_t x; asm volatile("csrr %0, sstatus" : "=r"(x)); return x; }
static inline void     w_sstatus(uint64_t x){ asm volatile("csrw sstatus, %0" :: "r"(x)); }
static inline uint64_t r_sie(void){ uint64_t x; asm volatile("csrr %0, sie" : "=r"(x)); return x; }
static inline void     w_sie(uint64_t x){ asm volatile("csrw sie, %0" :: "r"(x)); }
static inline void     w_stvec(uint64_t x){ asm volatile("csrw stvec, %0" :: "r"(x)); }
static inline uint64_t r_scause(void){ uint64_t x; asm volatile("csrr %0, scause" : "=r"(x)); return x; }
static inline uint64_t r_sepc(void){ uint64_t x; asm volatile("csrr %0, sepc" : "=r"(x)); return x; }
static inline void     w_sepc(uint64_t x){ asm volatile("csrw sepc, %0" :: "r"(x)); }
static inline uint64_t r_stval(void){ uint64_t x; asm volatile("csrr %0, stval" : "=r"(x)); return x; }
static inline uint64_t r_sip(void){ uint64_t x; asm volatile("csrr %0, sip" : "=r"(x)); return x; }
static inline void     w_sip(uint64_t x){ asm volatile("csrw sip, %0" :: "r"(x)); }
static inline uint64_t r_time(void){ uint64_t x; asm volatile("rdtime %0":"=r"(x)); return x; }

// ---------------- SSTATUS/SIE bits ----------------
#define SSTATUS_SIE   (1UL << 1)   // global S-mode interrupt enable
#define SIE_SEIE      (1UL << 9)   // external
#define SIE_STIE      (1UL << 5)   // timer
#define SIE_SSIE      (1UL << 1)   // software

// ---------------- scause decoding ----------------
#define SCAUSE_INTR_MASK   (1ULL << 63)
#define SCAUSE_CODE(x)     ((x) & 0xfffULL)
#define SCAUSE_SUPERVISOR_TIMER 5  // when interrupt bit set and code==5

// ---------------- convenience ----------------
static inline void intr_on(void){ w_sstatus(r_sstatus() | SSTATUS_SIE); }
static inline void intr_off(void){ w_sstatus(r_sstatus() & ~SSTATUS_SIE); }

// kernel/riscv.h
#pragma once
#include <stdint.h>

// ---------------- Machine CSR helpers ----------------
static inline uint64_t r_mstatus(void){ uint64_t x; asm volatile("csrr %0, mstatus" : "=r"(x)); return x; }
static inline void     w_mstatus(uint64_t x){ asm volatile("csrw mstatus, %0" :: "r"(x)); }
static inline uint64_t r_mie(void){ uint64_t x; asm volatile("csrr %0, mie" : "=r"(x)); return x; }
static inline void     w_mie(uint64_t x){ asm volatile("csrw mie, %0" :: "r"(x)); }
static inline uint64_t r_mip(void){ uint64_t x; asm volatile("csrr %0, mip" : "=r"(x)); return x; }
static inline void     w_mip(uint64_t x){ asm volatile("csrw mip, %0" :: "r"(x)); }
static inline uint64_t r_medeleg(void){ uint64_t x; asm volatile("csrr %0, medeleg" : "=r"(x)); return x; }
static inline void     w_medeleg(uint64_t x){ asm volatile("csrw medeleg, %0" :: "r"(x)); }
static inline uint64_t r_mideleg(void){ uint64_t x; asm volatile("csrr %0, mideleg" : "=r"(x)); return x; }
static inline void     w_mideleg(uint64_t x){ asm volatile("csrw mideleg, %0" :: "r"(x)); }
static inline uint64_t r_mtvec(void){ uint64_t x; asm volatile("csrr %0, mtvec" : "=r"(x)); return x; }
static inline void     w_mtvec(uint64_t x){ asm volatile("csrw mtvec, %0" :: "r"(x)); }
static inline uint64_t r_mepc(void){ uint64_t x; asm volatile("csrr %0, mepc" : "=r"(x)); return x; }
static inline void     w_mepc(uint64_t x){ asm volatile("csrw mepc, %0" :: "r"(x)); }
static inline uint64_t r_mcause(void){ uint64_t x; asm volatile("csrr %0, mcause" : "=r"(x)); return x; }
static inline uint64_t r_mhartid(void){ uint64_t x; asm volatile("csrr %0, mhartid" : "=r"(x)); return x; }
static inline void     w_mscratch(uint64_t x){ asm volatile("csrw mscratch, %0" :: "r"(x)); }
static inline uint64_t r_mcounteren(void){ uint64_t x; asm volatile("csrr %0, mcounteren" : "=r"(x)); return x; }
static inline void     w_mcounteren(uint64_t x){ asm volatile("csrw mcounteren, %0" :: "r"(x)); }
static inline void     w_pmpaddr0(uint64_t x){ asm volatile("csrw pmpaddr0, %0" :: "r"(x)); }
static inline void     w_pmpcfg0(uint64_t x){ asm volatile("csrw pmpcfg0, %0" :: "r"(x)); }

// ---------------- Supervisor CSR helpers ----------------
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
static inline void     w_satp(uint64_t x){ asm volatile("csrw satp, %0" :: "r"(x)); }

// ---------------- SSTATUS/SIE/SIP bits ----------------
#define SSTATUS_SIE   (1UL << 1)   // global S-mode interrupt enable
#define SIE_SEIE      (1UL << 9)   // external
#define SIE_STIE      (1UL << 5)   // timer
#define SIE_SSIE      (1UL << 1)   // software
#define SIP_SEIP      (1UL << 9)
#define SIP_STIP      (1UL << 5)
#define SIP_SSIP      (1UL << 1)

// ---------------- MSTATUS/MIE bits ----------------
#define MSTATUS_MIE       (1UL << 3)
#define MSTATUS_SIE       (1UL << 1)
#define MSTATUS_MPIE      (1UL << 7)
#define MSTATUS_SPIE      (1UL << 5)
#define MSTATUS_SPP       (1UL << 8)
#define MSTATUS_MPP_MASK  (3UL << 11)
#define MSTATUS_MPP_U     (0UL << 11)
#define MSTATUS_MPP_S     (1UL << 11)
#define MSTATUS_MPP_M     (3UL << 11)

#define MIE_MSIE          (1UL << 3)
#define MIE_MTIE          (1UL << 7)
#define MIE_MEIE          (1UL << 11)
#define MIE_SSIE          (1UL << 1)
#define MIE_STIE          (1UL << 5)
#define MIE_SEIE          (1UL << 9)

// ---------------- scause decoding ----------------
#define SCAUSE_INTR_MASK            (1ULL << 63)
#define SCAUSE_CODE(x)              ((x) & 0xfffULL)
#define SCAUSE_SUPERVISOR_SOFTWARE  1
#define SCAUSE_SUPERVISOR_TIMER     5
#define SCAUSE_SUPERVISOR_EXTERNAL  9

// ---------------- convenience ----------------
static inline void intr_on(void){ w_sstatus(r_sstatus() | SSTATUS_SIE); }
static inline void intr_off(void){ w_sstatus(r_sstatus() & ~SSTATUS_SIE); }

// ---------------- CLINT MMIO layout ----------------
#define CLINT_BASE              0x02000000UL
#define CLINT_MTIMECMP(hart)   (CLINT_BASE + 0x4000 + 8 * (hart))
#define CLINT_MTIME            (CLINT_BASE + 0xbff8)
#ifndef KVMINIT_H
#define KVMINIT_H

#include "pagetable.h"

/* Constants */
#define KERNBASE 0x80000000UL
#define MEMSIZE (128UL * 1024 * 1024)  // 128MB
#define UART0 0x10000000UL

/* Global kernel pagetable */
extern pagetable_t kernel_pagetable;

/* Functions */
void kvminit(void);
void kvminithart(void);
int map_region(pagetable_t pt, uint64_t va, uint64_t pa, uint64_t size, int perm);

#endif /* KVMINIT_H */


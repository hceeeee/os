#ifndef _PMM_H_
#define _PMM_H_

#include <stdint.h>

//#define PAGE_SIZE 4096

void pmm_init(uint64_t start, uint64_t end);
void* alloc_page(void);
void free_page(void* page);
void* alloc_pages(int n);

#endif

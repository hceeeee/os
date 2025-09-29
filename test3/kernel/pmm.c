// kernel/pmm.c
#include "pmm.h"
#include "printf.h"
#include <stdint.h>
#include <stddef.h>

#define MAX_PAGES 64   /* 64 页 = 256 KB */
#define PAGE_SIZE 4096

/* 静态内存池（位于内核数据段），页对齐 */
static unsigned char memory_pool[MAX_PAGES * PAGE_SIZE] __attribute__((aligned(4096)));

/* 简单栈式空闲页列表 */
static void *free_list[MAX_PAGES];
static int free_count = 0;

void pmm_init(uint64_t start, uint64_t end) {
    (void)start; // 未使用参数（目前我们用的是内核静态池）
    (void)end;

    free_count = 0;

    /* 初始化内存池 */
    for (int i = 0; i < MAX_PAGES; i++) {
        void *page = (void*)&memory_pool[i * PAGE_SIZE];
        free_list[free_count++] = page;
    }

    printf("PMM initialized: %d pages (%d KB)\n",
           MAX_PAGES, (MAX_PAGES * PAGE_SIZE) / 1024);
}

void* alloc_page(void) {
    if (free_count <= 0) {
        printf("pmm: out of memory!\n");
        return NULL;
    }
    void *page = free_list[--free_count];
    // 清零（可选）
    for (int i = 0; i < PAGE_SIZE; i++) {
        ((unsigned char*)page)[i] = 0;
    }
    printf("pmm: alloc_page -> %p (remain=%d)\n", page, free_count);
    return page;
}

void free_page(void* page) {
    if (!page) return;

    uintptr_t addr = (uintptr_t)page;
    uintptr_t pool_start = (uintptr_t)memory_pool;
    uintptr_t pool_end   = pool_start + (MAX_PAGES * PAGE_SIZE);

    if (addr < pool_start || addr >= pool_end) {
        printf("pmm: free_page: address %p out of pool\n", page);
        return;
    }
    if (addr % PAGE_SIZE != 0) {
        printf("pmm: free_page: address %p not aligned\n", page);
        return;
    }
    if (free_count >= MAX_PAGES) {
        printf("pmm: free_page: free list full (double free?)\n");
        return;
    }

    free_list[free_count++] = page;
    printf("pmm: free_page <- %p (remain=%d)\n", page, free_count);
}

void* alloc_pages(int n) {
    if (n <= 0) return NULL;
    if (free_count < n) {
        printf("pmm: alloc_pages(%d) failed (only %d free)\n", n, free_count);
        return NULL;
    }

    void *first = NULL;
    for (int i = 0; i < n; i++) {
        void *p = alloc_page();
        if (!p) {
            // 回滚
            for (int j = 0; j < i; j++) {
                free_page(((void**)free_list)[free_count]);
            }
            return NULL;
        }
        if (i == 0) first = p;
    }
    return first;
}

/**
 * kernel/pmm.c - 物理内存管理器 (Physical Memory Manager)
 * 
 * 实现原理：
 * 1. 使用静态内存池（64页 = 256KB）
 * 2. 栈式分配：空闲页面存储在数组中，通过索引管理
 * 3. 页对齐：所有分配返回的地址都是4KB对齐的
 * 
 * 核心设计决策：
 * - 静态池：简单可靠，适合实验环境
 * - 栈式分配：O(1)时间复杂度
 * - 固定大小：只支持页级分配，避免碎片问题
 */

#include "pmm.h"
#include "printf.h"
#include <stdint.h>
#include <stddef.h>

#define MAX_PAGES 64   /* 最大页数：64 页 = 256 KB */
#define PAGE_SIZE 4096 /* 页大小：4KB（与硬件页表一致） */

/* 静态内存池：位于内核数据段，4KB对齐确保页表使用 */
static unsigned char memory_pool[MAX_PAGES * PAGE_SIZE] __attribute__((aligned(4096)));

/* 空闲页栈：存储空闲页面的指针列表，free_count表示栈顶 */
static void *free_list[MAX_PAGES];
static int free_count = 0;

/**
 * pmm_init - 初始化物理内存管理器
 * 
 * 将所有页面添加到空闲列表，形成初始状态
 */
void pmm_init(uint64_t start, uint64_t end) {
    (void)start; // 未使用参数（目前我们用的是内核静态池）
    (void)end;

    free_count = 0;

    /* 遍历内存池，将每个页添加到空闲列表 */
    for (int i = 0; i < MAX_PAGES; i++) {
        void *page = (void*)&memory_pool[i * PAGE_SIZE];
        free_list[free_count++] = page;
    }

    printf("PMM initialized: %d pages (%d KB)\n",
           MAX_PAGES, (MAX_PAGES * PAGE_SIZE) / 1024);
}

/**
 * alloc_page - 分配一个物理页
 * 
 * 返回：分配成功返回页对齐的地址，失败返回NULL
 * 
 * 实现：从栈顶弹出页面，并清零
 */
void* alloc_page(void) {
    if (free_count <= 0) {
        printf("pmm: out of memory!\n");
        return NULL;
    }
    void *page = free_list[--free_count]; // 栈式分配：从栈顶取页
    // 清零页内容（安全性考虑）
    for (int i = 0; i < PAGE_SIZE; i++) {
        ((unsigned char*)page)[i] = 0;
    }
    printf("pmm: alloc_page -> %p (remain=%d)\n", page, free_count);
    return page;
}

/**
 * free_page - 释放一个物理页
 * 
 * 参数：page - 需要释放的页地址
 * 
 * 安全检查：
 * 1. 检查地址是否在内存池范围内
 * 2. 检查地址是否页对齐
 * 3. 检查是否重复释放
 */
void free_page(void* page) {
    if (!page) return;

    uintptr_t addr = (uintptr_t)page;
    uintptr_t pool_start = (uintptr_t)memory_pool;
    uintptr_t pool_end   = pool_start + (MAX_PAGES * PAGE_SIZE);

    /* 安全检查1：地址必须在内存池范围内 */
    if (addr < pool_start || addr >= pool_end) {
        printf("pmm: free_page: address %p out of pool\n", page);
        return;
    }
    /* 安全检查2：地址必须4KB对齐 */
    if (addr % PAGE_SIZE != 0) {
        printf("pmm: free_page: address %p not aligned\n", page);
        return;
    }
    /* 安全检查3：检测重复释放 */
    if (free_count >= MAX_PAGES) {
        printf("pmm: free_page: free list full (double free?)\n");
        return;
    }

    /* 将页压入空闲栈 */
    free_list[free_count++] = page;
    printf("pmm: free_page <- %p (remain=%d)\n", page, free_count);
}

/**
 * alloc_pages - 分配n个连续的物理页（用于页表结构）
 * 
 * 参数：n - 需要分配的页数
 * 返回：返回第一页的地址，失败返回NULL
 * 
 * 注意：虽然叫做"连续"，但实际不保证物理连续
 *      只是分配了n个页面，用于页表树结构
 */
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
            /* 失败回滚：释放已分配的所有页 */
            for (int j = 0; j < i; j++) {
                free_page(((void**)free_list)[free_count]);
            }
            return NULL;
        }
        if (i == 0) first = p;
    }
    return first;
}

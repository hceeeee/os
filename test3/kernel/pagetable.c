/**
 * kernel/pagetable.c - 页表管理系统
 * 
 * 实现原理：
 * 1. Sv39页表结构：3级页表（Level 2, 1, 0）
 * 2. 虚拟地址解析：39位地址 = 9位VPN[2] + 9位VPN[1] + 9位VPN[0] + 12位偏移
 * 3. 页表遍历：从根页表开始，逐级查找/创建中间表
 * 
 * 核心函数：
 * - walk_create: 遍历并创建中间页表（用于建立映射）
 * - walk_lookup: 遍历查找页表项（不创建，用于查找）
 * - map_page: 建立虚拟地址到物理地址的映射
 */

#include "pagetable.h"
#include "pmm.h"
#include "printf.h"
#include <stddef.h>
#include <stdint.h>

#define NPTE (PAGE_SIZE / sizeof(pte_t)) /* 每页表页包含512个PTE（9位索引） */

/**
 * pte_to_table - 从PTE中提取页表页的物理地址
 * 
 * PTE格式：PPN[53:10] | flags[9:0]
 * 将物理页号(PPN)左移12位得到物理地址
 */
static inline pagetable_t pte_to_table(pte_t pte) {
    uint64_t ppn = (pte >> PPN_SHIFT);  // 提取物理页号
    uint64_t addr = (ppn << 12);        // 转换为物理地址
    return (pagetable_t)addr;
}

/**
 * make_pte_for_table - 创建指向子页表的PTE
 * 
 * 用于在父页表中创建指向子页表页的PTE
 * 标志位：PTE_V（有效位）
 */
static inline pte_t make_pte_for_table(void* child_page) {
    uint64_t ppn = ((uint64_t)child_page) >> 12;  // 物理页号
    return (ppn << PPN_SHIFT) | PTE_V;             // 仅设置有效位
}

/**
 * make_leaf_pte - 创建叶子PTE（指向物理页）
 * 
 * 用于建立最终的虚拟地址到物理地址映射
 * 标志位：PTE_V（有效位）+ perm（权限位：R/W/X/U等）
 */
static inline pte_t make_leaf_pte(uint64_t pa, int perm) {
    uint64_t ppn = pa >> 12;                       // 物理页号
    return (ppn << PPN_SHIFT) | (uint64_t)(perm) | PTE_V;
}

/**
 * alloc_pagetable_page - 分配并清零一个页表页
 * 
 * 页表页必须清零，确保所有PTE初始为0（无效）
 */
static void* alloc_pagetable_page(void) {
    void *p = alloc_page();  // 从PMM分配页面
    if (!p) return NULL;
    unsigned char *b = (unsigned char*)p;
    for (size_t i = 0; i < PAGE_SIZE; i++) b[i] = 0;  // 清零
    return p;
}

/**
 * create_pagetable - 创建新的根页表
 * 
 * 返回：分配好的页表页指针
 * 
 * 页表本身就是一页内存，包含512个64位PTE项
 */
pagetable_t create_pagetable(void) {
    void *p = alloc_pagetable_page();
    return (pagetable_t)p;
}

/**
 * walk_create - 页表遍历并创建（用于建立映射）
 * 
 * 功能：从根页表遍历到Level 0，创建所有需要的中间页表
 * 
 * 工作流程（以Sv39为例，VA=0x40000000）：
 * 1. Level 2 (VPN[2])：检查根页表项，不存在则创建
 * 2. Level 1 (VPN[1])：进入中间页表，检查并创建
 * 3. Level 0 (VPN[0])：返回指向最终PTE的指针
 * 
 * 安全机制：
 * - 检查冲突：如果遇到已存在的叶子映射，返回NULL
 * - 自动分配：中间页表不存在时自动创建
 */
pte_t* walk_create(pagetable_t pt, uint64_t va) {
    if (!pt) return NULL;
    pagetable_t table = pt;
    
    /* 从Level 2到Level 1，逐级遍历 */
    for (int level = 2; level > 0; level--) {
        /* 提取当前级别的页表索引（9位）*/
        uint64_t idx = VPN_MASK(va, level);
        pte_t pte = table[idx];
        
        if (pte & PTE_V) {
            /* PTE有效，检查是否为叶子映射（冲突检测）*/
            if (pte & (PTE_R|PTE_W|PTE_X)) {
                /* 已经是叶子映射，不能继续创建子表 */
                return NULL;
            }
            /* 非叶子：进入下一级页表 */
            table = pte_to_table(pte);
        } else {
            /* PTE无效，需要创建子页表页 */
            void *child = alloc_pagetable_page();
            if (!child) return NULL;
            /* 在父页表中创建指向子页表的PTE */
            table[idx] = make_pte_for_table(child);
            /* 切换到子页表继续遍历 */
            table = (pagetable_t)child;
        }
    }
    /* 返回Level 0的PTE指针，用于设置最终的映射 */
    return &table[VPN_MASK(va, 0)];
}

/**
 * walk_lookup - 页表查找（不创建）
 * 
 * 功能：查找虚拟地址对应的PTE
 * 
 * 与walk_create的区别：
 * - walk_create: 创建不存在的中间表
 * - walk_lookup: 只查找，不创建
 * 
 * 返回值：
 * - 成功：返回PTE指针（可能在任意层级）
 * - 失败：返回NULL
 */
pte_t* walk_lookup(pagetable_t pt, uint64_t va) {
    if (!pt) return NULL;
    pagetable_t table = pt;
    
    for (int level = 2; level > 0; level--) {
        uint64_t idx = VPN_MASK(va, level);
        pte_t pte = table[idx];
        
        if (!(pte & PTE_V)) return NULL;  // 无效PTE，映射不存在
        
        if (pte & (PTE_R|PTE_W|PTE_X)) {
            /* 找到叶子映射（在更高层级）*/
            return &table[idx];
        }
        /* 继续下一级 */
        table = pte_to_table(pte);
    }
    /* 到达Level 0，返回最终PTE */
    return &table[VPN_MASK(va, 0)];
}

/**
 * map_page - 建立虚拟地址到物理地址的页映射
 * 
 * 参数：
 * - pt: 页表指针
 * - va: 虚拟地址（必须页对齐）
 * - pa: 物理地址（必须页对齐）
 * - perm: 权限位（PTE_R | PTE_W | PTE_X等）
 * 
 * 返回值：成功返回0，失败返回-1
 * 
 * 工作流程：
 * 1. 检查地址对齐
 * 2. 遍历页表找到目标PTE（自动创建中间表）
 * 3. 检查是否已映射（避免重复映射）
 * 4. 设置PTE的物理页号和权限
 */
int map_page(pagetable_t pt, uint64_t va, uint64_t pa, int perm) {
    /* 1. 地址对齐检查：必须是4KB边界 */
    if ((va & (PAGE_SIZE-1)) || (pa & (PAGE_SIZE-1))) {
        printf("map_page: addresses must be page aligned\n");
        return -1;
    }
    
    /* 2. 遍历页表找到目标PTE（自动创建中间页表）*/
    pte_t *pte = walk_create(pt, va);
    if (!pte) {
        printf("map_page: walk_create failed for va %p\n", (void*)va);
        return -1;
    }
    
    /* 3. 检查是否已映射（防止重复映射）*/
    if ((*pte & PTE_V) && (*pte & (PTE_R|PTE_W|PTE_X))) {
        printf("map_page: VA %p already mapped\n", (void*)va);
        return -1;
    }
    
    /* 4. 创建叶子PTE，设置映射 */
    *pte = make_leaf_pte(pa, perm);
    return 0;
}

/* destroy_level: recursively destroy page table pages (only page-table pages).
   We DO NOT free physical pages that were mapped as leaves here. */
static void destroy_level(pagetable_t table, int level) {
    if (!table) return;
    for (int i = 0; i < NPTE; i++) {
        pte_t pte = table[i];
        if (!(pte & PTE_V)) continue;
        /* If it's a leaf (has R/W/X), skip (do not free mapped physical memory) */
        if ( (pte & (PTE_R|PTE_W|PTE_X)) ) {
            table[i] = 0;
            continue;
        }
        /* Non-leaf: child page table */
        pagetable_t child = pte_to_table(pte);
        /* clear entry first to avoid re-entrance issues */
        table[i] = 0;
        /* recurse into child and then free the child page */
        destroy_level(child, level - 1);
        free_page((void*)child);
    }
}

/* destroy entire pagetable rooted at pt (including freeing root page) */
void destroy_pagetable(pagetable_t pt) {
    if (!pt) return;
    destroy_level(pt, 2);
    free_page((void*)pt);
}

/* Dump helpers: compute va_base at this level and recurse */
static void dump_level(pagetable_t table, int level, uint64_t va_base) {
    if (!table) return;
    for (int i = 0; i < NPTE; i++) {
        pte_t pte = table[i];
        if (!(pte & PTE_V)) continue;
        uint64_t va = va_base | ((uint64_t)i << VPN_SHIFT(level));
        if (pte & (PTE_R|PTE_W|PTE_X)) {
            uint64_t pa = (pte >> PPN_SHIFT) << 12;
            int perm = pte & (PTE_R|PTE_W|PTE_X);
            printf("MAP: va=%p -> pa=%p perm=0x%x\n", (void*)va, (void*)pa, perm);
        } else {
            /* non-leaf: recurse to child */
            pagetable_t child = pte_to_table(pte);
            dump_level(child, level - 1, va);
        }
    }
}

void dump_pagetable(pagetable_t pt) {
    printf("Dump pagetable:\n");
    dump_level(pt, 2, 0UL);
}
